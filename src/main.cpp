// Virtual Sensor HIL Telemetry Engine
// ----------------------------------------------------------------------------
//   control thread :  read sensor -> check health -> PID -> set heater   (every 100 ms)
//   main thread    :  small web server, shows the data in the browser
//   shared data    :  protected by one mutex
//
//   Browser  <--HTTP-->  [web server]  <--shared data-->  [control thread]
//                                                               |  read()/ioctl()
//                                                       /dev/vsensor0 (kernel driver)
//                                                       or software simulator
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include "control.hpp"
#include "dashboard_page.hpp"
#include "sensor_source.hpp"
#include "web_server.hpp"

static volatile std::sig_atomic_t g_stop = 0;
static void onSignal(int) { g_stop = 1; }          // Ctrl+C -> just set a flag

// ---------------------------------------------------------------- shared data
struct Shared {
    std::mutex lock;                      // protects everything below
    std::deque<Sample> history;           // last 300 samples (for the charts)
    std::deque<std::string> events;       // last 50 log lines
    double setpoint = 60.0;               // set from the browser
    unsigned fault = 0;                   // 0 none, 1 stuck, 2 spike, 3 dropout
    bool faultChanged = false;
    unsigned long samples = 0, dropouts = 0, anomalies = 0;
    std::string source;
};

static void addEvent(Shared& sh, const std::string& text) {       // caller holds the lock
    sh.events.push_back(text);
    if (sh.events.size() > 50) sh.events.pop_front();
}

// ------------------------------------------------------------ control thread
// This is the "loop" of Hardware-in-the-Loop. Only this thread touches the device.
static void controlLoop(ISensorSource& dev, Shared& sh, int periodMs, const char* csvPath) {
    PID pid(8.0, 0.4, 1.0);
    HealthChecker health;
    FILE* csv = std::fopen(csvPath, "w");
    if (csv) std::fputs("seq,temp_c,hum_pct,press_pa,heater_pct,setpoint_c,anomaly\n", csv);

    const double dt = periodMs / 1000.0;
    double heater = 0;
    Anomaly previous = Anomaly::None;
    auto next = std::chrono::steady_clock::now();

    while (!g_stop) {
        next += std::chrono::milliseconds(periodMs);          // fixed rate, no drift
        std::this_thread::sleep_until(next);

        // 1) apply commands that came from the browser
        double setpoint;
        {
            std::lock_guard<std::mutex> g(sh.lock);
            setpoint = sh.setpoint;
            if (sh.faultChanged) { dev.setFault(sh.fault); sh.faultChanged = false; }
        }

        // 2) read the sensor
        Sample s;
        ReadStatus status = dev.read(s);
        if (status == ReadStatus::Fatal) { g_stop = 1; break; }
        if (status == ReadStatus::Dropout) {                  // no data: keep last heater value
            std::lock_guard<std::mutex> g(sh.lock);
            ++sh.dropouts;
            if (sh.dropouts % 10 == 1) addEvent(sh, "sensor read failed (dropout) - holding heater output");
            continue;
        }

        // 3) is the reading believable?
        s.anomaly = health.check(s.temp_c);
        s.setpoint_c = setpoint;

        // 4) control: PID on the last GOOD temperature, with an over-temperature safety cut-off
        double usable = health.haveGood() ? health.lastGood() : s.temp_c;
        if (s.temp_c > 100.0) { heater = 0; pid.reset(); s.anomaly = Anomaly::Interlock; }
        else if (health.haveGood()) heater = pid.update(setpoint, usable, dt);

        // 5) drive the actuator
        dev.setHeater(static_cast<unsigned>(std::lround(heater)));
        s.heater_pct = std::round(heater);

        // 6) publish for the web page + CSV log
        {
            std::lock_guard<std::mutex> g(sh.lock);
            sh.history.push_back(s);
            if (sh.history.size() > 300) sh.history.pop_front();
            ++sh.samples;
            if (s.anomaly != Anomaly::None) {
                ++sh.anomalies;
                if (s.anomaly != previous || s.anomaly == Anomaly::Spike) {
                    char msg[128];
                    std::snprintf(msg, sizeof(msg), "seq %u: %s detected (reading %.2f C) - using last good value",
                                  s.seq, toString(s.anomaly), s.temp_c);
                    addEvent(sh, msg);
                }
            }
        }
        previous = s.anomaly;
        if (csv) std::fprintf(csv, "%u,%.3f,%.2f,%.0f,%.0f,%.1f,%s\n", s.seq, s.temp_c, s.hum_pct,
                              s.press_pa, s.heater_pct, s.setpoint_c, toString(s.anomaly));
    }
    dev.setHeater(0);
    if (csv) std::fclose(csv);
}

// ----------------------------------------------------------------- JSON for /api/data
static std::string buildJson(Shared& sh) {
    static const char* faultNames[] = {"none", "stuck", "spike", "dropout"};
    std::lock_guard<std::mutex> g(sh.lock);
    std::string j = "{\"source\":\"" + sh.source + "\",\"fault\":\"" + faultNames[sh.fault] + "\"";
    char buf[256];
    std::snprintf(buf, sizeof(buf), ",\"setpoint\":%.1f,\"stats\":{\"samples\":%lu,\"dropouts\":%lu,\"anomalies\":%lu}",
                  sh.setpoint, sh.samples, sh.dropouts, sh.anomalies);
    j += buf;
    j += ",\"samples\":[";
    for (std::size_t i = 0; i < sh.history.size(); ++i) {
        const Sample& s = sh.history[i];
        std::snprintf(buf, sizeof(buf),
            "%s{\"seq\":%u,\"temp\":%.3f,\"hum\":%.2f,\"press\":%.0f,\"heater\":%.0f,\"setpoint\":%.1f,\"anomaly\":\"%s\"}",
            i ? "," : "", s.seq, s.temp_c, s.hum_pct, s.press_pa, s.heater_pct, s.setpoint_c, toString(s.anomaly));
        j += buf;
    }
    j += "],\"events\":[";
    for (std::size_t i = 0; i < sh.events.size(); ++i) j += (i ? ",\"" : "\"") + sh.events[i] + "\"";
    j += "]}";
    return j;
}

// ----------------------------------------------------------------- web routes
static HttpResponse route(Shared& sh, const std::string& path) {
    HttpResponse r;
    if (path == "/" || path == "/index.html") {
        r.contentType = "text/html; charset=utf-8";
        r.body = DASHBOARD_HTML;
    } else if (path.rfind("/api/data", 0) == 0) {
        r.contentType = "application/json";
        r.body = buildJson(sh);
    } else if (path.rfind("/api/setpoint", 0) == 0) {
        double v = std::atof(queryParam(path, "value").c_str());
        if (v < 30.0 || v > 95.0) { r.status = 400; r.body = "setpoint must be 30..95"; return r; }
        std::lock_guard<std::mutex> g(sh.lock);
        sh.setpoint = v;
        addEvent(sh, "setpoint changed to " + std::to_string(static_cast<int>(v)) + " C");
        r.body = "ok";
    } else if (path.rfind("/api/fault", 0) == 0) {
        std::string m = queryParam(path, "mode");
        unsigned code = m == "none" ? 0 : m == "stuck" ? 1 : m == "spike" ? 2 : m == "dropout" ? 3 : 99;
        if (code == 99) { r.status = 400; r.body = "unknown fault mode"; return r; }
        std::lock_guard<std::mutex> g(sh.lock);
        sh.fault = code; sh.faultChanged = true;
        addEvent(sh, "fault injection: " + m);
        r.body = "ok";
    } else {
        r.status = 404; r.body = "not found";
    }
    return r;
}

// ----------------------------------------------------------------------- main
int main(int argc, char** argv) {
    std::string mode = "auto", bindIp = "127.0.0.1", csvPath = "telemetry.csv";
    int port = 8080, periodMs = 100;
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string a = argv[i], v = argv[i + 1];
        if (a == "--mode") mode = v;                 // auto | driver | sim
        else if (a == "--port") port = std::atoi(v.c_str());
        else if (a == "--bind") bindIp = v;          // use 0.0.0.0 to open from another PC
        else if (a == "--period-ms") periodMs = std::atoi(v.c_str());
        else if (a == "--log") csvPath = v;
        else { std::printf("usage: %s [--mode auto|driver|sim] [--port N] [--bind IP] [--period-ms N] [--log FILE]\n", argv[0]); return 1; }
    }
    if (periodMs < 10) periodMs = 10;

    struct sigaction sa{};
    sa.sa_handler = onSignal;                        // no SA_RESTART
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // choose the data source: real kernel driver if available, otherwise the simulator
    std::unique_ptr<ISensorSource> dev;
    if (mode != "sim") {
        dev = makeDriverSource("/dev/vsensor0");
        if (!dev->open()) {
            if (mode == "driver") { std::fprintf(stderr, "cannot open /dev/vsensor0 - load the driver first (scripts/load_driver.sh)\n"); return 2; }
            dev.reset();
        }
    }
    if (!dev) { dev = makeSimSource(); dev->open(); }

    Shared shared;
    shared.source = dev->name();
    addEvent(shared, std::string("engine started, source: ") + dev->name());

    std::thread control(controlLoop, std::ref(*dev), std::ref(shared), periodMs, csvPath.c_str());

    std::printf("Source : %s\nOpen in browser:  http://%s:%d\nPress Ctrl+C to stop.\n",
                dev->name(), bindIp == "0.0.0.0" ? "localhost" : bindIp.c_str(), port);
    bool ok = runWebServer(bindIp, port, [&](const std::string& p) { return route(shared, p); }, g_stop);
    if (!ok) { std::fprintf(stderr, "cannot start web server on %s:%d (port in use?)\n", bindIp.c_str(), port); g_stop = 1; }

    g_stop = 1;
    control.join();
    std::printf("stopped. CSV log: %s\n", csvPath.c_str());
    return ok ? 0 : 1;
}
