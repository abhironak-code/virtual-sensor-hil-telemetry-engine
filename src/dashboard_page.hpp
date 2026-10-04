#pragma once

static const char* const DASHBOARD_HTML = R"HTML(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Virtual Sensor HIL Telemetry Engine</title>
<style>
  :root{--bg:#0d1117;--card:#161b22;--line:#30363d;--text:#e6edf3;--mute:#8b949e;
        --red:#ff7b72;--orange:#ffa657;--blue:#79c0ff;--purple:#d2a8ff;--green:#7ee787}
  *{box-sizing:border-box} body{margin:0;background:var(--bg);color:var(--text);
        font-family:system-ui,Segoe UI,Arial,sans-serif}
  header{display:flex;justify-content:space-between;align-items:center;padding:14px 22px;
        border-bottom:1px solid var(--line)}
  h1{font-size:18px;margin:0} h1 small{color:var(--mute);font-weight:normal;margin-left:8px}
  #conn{font-size:13px;color:var(--green)}
  main{padding:18px 22px;max-width:1300px;margin:auto}
  .row{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:12px;margin-bottom:14px}
  .card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px 14px}
  .card .label{font-size:11px;letter-spacing:.08em;color:var(--mute)}
  .card .value{font-size:26px;font-weight:700;margin-top:4px}
  .controls{display:flex;flex-wrap:wrap;gap:18px;align-items:center}
  .controls label{color:var(--mute);font-size:13px;margin-right:6px}
  input[type=number]{width:80px;background:var(--bg);color:var(--text);border:1px solid var(--line);
        border-radius:6px;padding:6px}
  button{background:#238636;color:#fff;border:0;border-radius:6px;padding:7px 14px;font-weight:600;cursor:pointer}
  button.f{background:#30363d} button.f.on{background:#da3633}
  .charts{display:grid;grid-template-columns:repeat(auto-fit,minmax(420px,1fr));gap:12px;margin:14px 0}
  .charts .card{padding:8px}
  canvas{width:100%;height:210px;display:block}
  #events{height:130px;overflow:auto;font-family:ui-monospace,Consolas,monospace;font-size:12px;
        background:#010409;border-radius:6px;padding:8px;white-space:pre-wrap}
  #stats{color:var(--mute);font-size:13px;margin:6px 2px 12px}
</style></head>
<body>
<header>
  <h1>Virtual Sensor HIL Telemetry Engine<small>Domain 1 - IoT, Embedded &amp; Virtual Sensors</small></h1>
  <span id="conn">connecting...</span>
</header>
<main>
  <div class="row">
    <div class="card"><div class="label">TEMPERATURE</div><div class="value" id="vT" style="color:var(--red)">--</div></div>
    <div class="card"><div class="label">HEATER (PID OUTPUT)</div><div class="value" id="vH" style="color:var(--orange)">--</div></div>
    <div class="card"><div class="label">HUMIDITY</div><div class="value" id="vHu" style="color:var(--blue)">--</div></div>
    <div class="card"><div class="label">PRESSURE</div><div class="value" id="vP" style="color:var(--purple)">--</div></div>
    <div class="card"><div class="label">DATA SOURCE</div><div class="value" id="vSrc" style="font-size:16px;color:var(--green)">--</div></div>
    <div class="card"><div class="label">SENSOR HEALTH</div><div class="value" id="vHealth" style="color:var(--green)">--</div></div>
  </div>

  <div class="card controls">
    <div><label>Setpoint (&deg;C)</label><input id="sp" type="number" min="30" max="95" value="60">
         <button onclick="setSetpoint()">Set</button></div>
    <div><label>Inject fault:</label>
         <button class="f" id="f-none"    onclick="setFault('none')">None</button>
         <button class="f" id="f-spike"   onclick="setFault('spike')">Spike</button>
         <button class="f" id="f-stuck"   onclick="setFault('stuck')">Stuck</button>
         <button class="f" id="f-dropout" onclick="setFault('dropout')">Dropout</button></div>
  </div>

  <div class="charts">
    <div class="card"><canvas id="cTemp"></canvas></div>
    <div class="card"><canvas id="cHeat"></canvas></div>
    <div class="card"><canvas id="cHum"></canvas></div>
    <div class="card"><canvas id="cPres"></canvas></div>
  </div>

  <div id="stats"></div>
  <div class="card"><div class="label" style="margin-bottom:6px">EVENT LOG</div><div id="events"></div></div>
</main>

<script>
const $ = id => document.getElementById(id);

// ---- 1. Ask the C++ server for the latest data and show it -----------------
async function refresh() {
  try {
    const res = await fetch('/api/data');
    show(await res.json());
    $('conn').textContent = 'connected'; $('conn').style.color = '#7ee787';
  } catch (e) {
    $('conn').textContent = 'server offline'; $('conn').style.color = '#ff7b72';
  }
}
setInterval(refresh, 200);          // 5 times per second

function show(d) {
  const s = d.samples, last = s[s.length - 1];
  if (last) {
    $('vT').textContent  = last.temp.toFixed(2) + ' \u00B0C';
    $('vH').textContent  = last.heater.toFixed(0) + ' %';
    $('vHu').textContent = last.hum.toFixed(1) + ' %';
    $('vP').textContent  = last.press.toFixed(0) + ' Pa';
    const bad = last.anomaly !== 'none';
    $('vHealth').textContent = bad ? last.anomaly.toUpperCase() : 'NORMAL';
    $('vHealth').style.color = bad ? '#ff7b72' : '#7ee787';
  }
  $('vSrc').textContent = d.source;
  for (const f of ['none', 'spike', 'stuck', 'dropout'])
    $('f-' + f).classList.toggle('on', f === d.fault);

  drawChart($('cTemp'), 'Temperature vs Setpoint (\u00B0C)', [
    { values: s.map(x => x.temp), color: '#ff7b72', marks: s.map(x => x.anomaly !== 'none') },
    { values: s.map(x => x.setpoint), color: '#7ee787', dashed: true }]);
  drawChart($('cHeat'), 'Heater output - PID (%)', [{ values: s.map(x => x.heater), color: '#ffa657' }]);
  drawChart($('cHum'),  'Humidity (%RH)',          [{ values: s.map(x => x.hum),    color: '#79c0ff' }]);
  drawChart($('cPres'), 'Pressure (Pa)',           [{ values: s.map(x => x.press),  color: '#d2a8ff' }]);

  $('stats').textContent = 'samples: ' + d.stats.samples + '   dropouts: ' + d.stats.dropouts +
                           '   anomalies: ' + d.stats.anomalies + '   setpoint: ' + d.setpoint + ' \u00B0C';
  const box = $('events'), atBottom = box.scrollTop + box.clientHeight >= box.scrollHeight - 5;
  box.textContent = d.events.join('\n');
  if (atBottom) box.scrollTop = box.scrollHeight;
}

// ---- 2. Send commands to the server ----------------------------------------
function setSetpoint() { fetch('/api/setpoint?value=' + $('sp').value); }
function setFault(mode) { fetch('/api/fault?mode=' + mode); }

// ---- 3. A small line-chart function (plain canvas, no libraries) -----------
function drawChart(canvas, title, series) {
  const dpr = window.devicePixelRatio || 1;
  const W = canvas.clientWidth, H = canvas.clientHeight;
  canvas.width = W * dpr; canvas.height = H * dpr;
  const g = canvas.getContext('2d');
  g.scale(dpr, dpr);

  g.fillStyle = '#e6edf3'; g.font = 'bold 13px sans-serif'; g.fillText(title, 10, 18);
  const L = 56, R = W - 12, T = 30, B = H - 14;                 // plot area

  // find the min / max of all values for the y-axis
  let lo = Infinity, hi = -Infinity;
  for (const s of series) for (const v of s.values) { lo = Math.min(lo, v); hi = Math.max(hi, v); }
  if (!isFinite(lo)) return;
  if (hi - lo < 1) { lo -= 0.5; hi += 0.5; }
  const pad = (hi - lo) * 0.1; lo -= pad; hi += pad;

  // grid lines + y labels
  g.font = '11px sans-serif'; g.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const y = B - (B - T) * i / 4, v = lo + (hi - lo) * i / 4;
    g.strokeStyle = '#21262d'; g.beginPath(); g.moveTo(L, y); g.lineTo(R, y); g.stroke();
    g.fillStyle = '#8b949e'; g.textAlign = 'right';
    g.fillText(v.toFixed(hi - lo < 10 ? 2 : (hi - lo < 100 ? 1 : 0)), L - 6, y + 4);
  }
  g.textAlign = 'left';

  // the lines (newest value on the right edge)
  const MAX = 300;
  for (const s of series) {
    g.strokeStyle = s.color; g.lineWidth = 2; g.setLineDash(s.dashed ? [6, 4] : []);
    g.beginPath();
    s.values.forEach((v, i) => {
      const x = R - (s.values.length - 1 - i) * (R - L) / (MAX - 1);
      const y = B - (v - lo) / (hi - lo) * (B - T);
      i ? g.lineTo(x, y) : g.moveTo(x, y);
    });
    g.stroke(); g.setLineDash([]);
    if (s.marks) {                                    // red dots = detected anomalies
      g.fillStyle = '#ff4d4d';
      s.values.forEach((v, i) => {
        if (!s.marks[i]) return;
        const x = R - (s.values.length - 1 - i) * (R - L) / (MAX - 1);
        const y = B - (v - lo) / (hi - lo) * (B - T);
        g.beginPath(); g.arc(x, y, 4, 0, 6.3); g.fill();
      });
    }
  }
}
refresh();
</script></body></html>)HTML";
