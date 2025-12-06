import request from "supertest";
import { describe, test, beforeEach } from "node:test";
import assert from "assert";
import { app } from "../app.js";

describe("API Endpoints", () => {
  beforeEach(async () => {
    await request(app).post("/api/sensors/reset");
  });

  test("GET /api/sensors/test returns test data", async () => {
    const res = await request(app).get("/api/sensors/test");
    assert.strictEqual(res.statusCode, 200);
    assert(Array.isArray(res.body));
    assert("temperature" in res.body[0]);
  });

  test("POST /api/sensors with valid data", async () => {
    const payload = { temperature: 21.5, humidity: 50, led_temp: 22.0 };
    const res = await request(app).post("/api/sensors").send(payload);
    assert.strictEqual(res.statusCode, 201);
    assert.strictEqual(res.body.status, "ok");
  });

  test("POST /api/sensors with missing data returns 400", async () => {
    const res = await request(app).post("/api/sensors").send({});
    assert.strictEqual(res.statusCode, 400);
    assert("error" in res.body);
  });

  test("GET /api/sensors returns sensor data", async () => {
    const res = await request(app).get("/api/sensors");
    assert.strictEqual(res.statusCode, 200);
    assert(Array.isArray(res.body));
  });

  test("GET /api/sensors returns correct amount of sensor data", async () => {
    for (let i = 0; i < 5; i++) {
      await request(app)
        .post("/api/sensors")
        .send({ temperature: 20 + i, humidity: 50 + i, led_temp: 21 + i });
    }

    const res = await request(app).get("/api/sensors");
    assert.strictEqual(res.statusCode, 200);
    assert(Array.isArray(res.body));
    assert.strictEqual(res.body.length, 5);
  });

  test("POST /api/command with valid fan_limits", async () => {
    const cmd = { type: "fan_limits", min_temp: 20, max_temp: 30 };
    const res = await request(app).post("/api/command").send(cmd);
    assert.strictEqual(res.statusCode, 200);
    assert.strictEqual(res.body.ok, true);
  });

  test("POST /api/command with invalid fan_limits returns 400", async () => {
    const cmd = { type: "fan_limits", min_temp: 40, max_temp: 30 };
    const res = await request(app).post("/api/command").send(cmd);
    assert.strictEqual(res.statusCode, 400);
    assert("error" in res.body);
  });

  test("POST /api/sensors returns just sent fan limits in commands", async () => {
    // Post new fan limits
    const fanLimits = { type: "fan_limits", min_temp: 18, max_temp: 25 };
    await request(app).post("/api/command").send(fanLimits);

    //  Post sensor data
    const sensorPayload = { temperature: 21.5, humidity: 50, led_temp: 22.0 };
    const res = await request(app).post("/api/sensors").send(sensorPayload);

    // Check response includes the just sent fan limits in commands
    assert.strictEqual(res.statusCode, 201);
    assert(Array.isArray(res.body.commands));
    const found = res.body.commands.some(
      (cmd) =>
        cmd.type === "fan_limits" &&
        cmd.min_temp === fanLimits.min_temp &&
        cmd.max_temp === fanLimits.max_temp
    );
    assert(found, "Fan limits command should be present in response");
  });
});
