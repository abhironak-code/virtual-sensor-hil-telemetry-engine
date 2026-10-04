#!/bin/sh
# Integration test: starts the real server (simulator mode) and checks the web API.
# Needs: curl.   Run with:  make itest
PORT=8099
BIN=./build/bin/hil_server
PASS=0; FAIL=0

check() {   # check "description" "command that must succeed"
    if sh -c "$2" >/dev/null 2>&1; then echo "  PASS  $1"; PASS=$((PASS+1));
    else echo "  FAIL  $1"; FAIL=$((FAIL+1)); fi
}

$BIN --mode sim --port $PORT --log /tmp/itest.csv >/dev/null 2>&1 &
PID=$!
sleep 4
URL=http://127.0.0.1:$PORT

echo "Integration tests (server pid $PID)"
check "GET / returns the dashboard page"        "curl -s $URL/ | grep -q 'Virtual Sensor'"
check "GET /api/data returns samples"           "curl -s $URL/api/data | grep -q '\"samples\":\[{'"
check "data source is the simulator"            "curl -s $URL/api/data | grep -q 'software-simulator'"
check "setpoint 70 is accepted"                 "curl -s $URL/api/setpoint?value=70 | grep -q ok"
check "setpoint 500 is rejected (HTTP 400)"     "[ \$(curl -s -o /dev/null -w '%{http_code}' $URL/api/setpoint?value=500) = 400 ]"
check "unknown path gives HTTP 404"             "[ \$(curl -s -o /dev/null -w '%{http_code}' $URL/nothing) = 404 ]"
check "unknown fault mode is rejected"          "[ \$(curl -s -o /dev/null -w '%{http_code}' '$URL/api/fault?mode=xyz') = 400 ]"

curl -s "$URL/api/fault?mode=spike" >/dev/null; sleep 3
check "spike fault is detected as anomaly"      "curl -s $URL/api/data | grep -q '\"anomaly\":\"spike\"'"
curl -s "$URL/api/fault?mode=stuck" >/dev/null; sleep 3
check "stuck fault is detected as anomaly"      "curl -s $URL/api/data | grep -q '\"anomaly\":\"stuck\"'"
curl -s "$URL/api/fault?mode=dropout" >/dev/null; sleep 3
check "dropout fault is counted"                "! curl -s $URL/api/data | grep -q '\"dropouts\":0'"
curl -s "$URL/api/fault?mode=none" >/dev/null

kill -INT $PID; wait $PID; RC=$?
check "server stops cleanly on Ctrl+C (exit 0)" "[ $RC = 0 ]"
check "CSV log file was written"                "[ \$(wc -l < /tmp/itest.csv) -gt 10 ]"

echo "Result: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
