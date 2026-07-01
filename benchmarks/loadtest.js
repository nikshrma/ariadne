import http from "k6/http";
import { check, sleep } from "k6";
import { Trend } from "k6/metrics";

const latencyTrend = new Trend("custom_latency");

export const options = {
  scenarios: {
    steady_load: {
      executor: "constant-vus",
      vus: 50,
      duration: "2m",
    },
    ramping_load: {
      executor: "ramping-vus",
      startVUs: 0,
      stages: [
        { duration: "30s", target: 50 },
        { duration: "1m", target: 200 },
        { duration: "30s", target: 200 },
        { duration: "30s", target: 0 },
      ],
      startTime: "2m30s",
    },
  },
  thresholds: {
    http_req_duration: ["p(95)<500", "p(99)<1000"],
    http_req_failed: ["rate<0.01"],
  },
};

const BASE_URL = "http://localhost:3000";
const okOr500 = { responseCallback: http.expectedStatuses(200, 500) };

export default function () {
  const roll = Math.random();
  let res;

  if (roll < 0.4) {
    res = http.get(`${BASE_URL}/`, okOr500);
    check(res, { "GET / status 200": (r) => r.status === 200 });
  } else if (roll < 0.7) {
    res = http.get(`${BASE_URL}/api/user`, okOr500);
    check(res, {
      "GET /api/user status 200": (r) => r.status === 200,
      "GET /api/user has body": (r) => r.body && r.body.length > 0,
    });
  } else if (roll < 0.95) {
    const payload = JSON.stringify({ test: "data", ts: Date.now() });
    res = http.post(`${BASE_URL}/submit`, payload, {
      headers: { "Content-Type": "application/json" },
      ...okOr500,
    });
    check(res, { "POST /submit status 200": (r) => r.status === 200 });
  } else {
    res = http.get(`${BASE_URL}/error`, okOr500);
    check(res, { "GET /error status 500": (r) => r.status === 500 });
  }

  latencyTrend.add(res.timings.duration);
  sleep(0.1);
}
