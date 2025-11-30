import express from "express";
import sqlite3 from "sqlite3";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const port = process.env.PORT || 3000;

app.use(express.json());

// Serve static frontend from ./public
app.use(express.static(path.join(__dirname, "public")));
let commands = [];

const db = new sqlite3.Database("./mydb.sqlite3", (err) => {
  if (err) return console.error(err.message);
  console.log("Connected to SQLite database");
});

db.run(`CREATE TABLE IF NOT EXISTS sensors (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  temperature REAL,
  humidity REAL,
  led_temp REAL,
  date DATETIME DEFAULT CURRENT_TIMESTAMP
)`);

// Health/test endpoint
app.get("/api/sensors/test", (req, res) => {
  res.json([
    {
      id: 1234,
      temperature: 22.5,
      humidity: 55,
      led_temp: 22.6,
      date: "2025-10-01 15:00:00",
    },
  ]);
});

app.get("/api/sensors", (req, res) => {
  let { limit = 200, from, to } = req.query;

  limit = Math.min(Math.max(parseInt(limit) || 200, 1), 10000);

  const MIN_DATE = new Date("2000-01-01").getTime();
  const MAX_DATE = new Date("2050-01-01").getTime();

  if (from) {
    const fromTime = new Date(from).getTime();
    if (isNaN(fromTime) || fromTime < MIN_DATE || fromTime > MAX_DATE) {
      return res.status(400).json({ error: "Invalid 'from' date" });
    }
  }

  if (to) {
    const toTime = new Date(to).getTime();
    if (isNaN(toTime) || toTime < MIN_DATE || toTime > MAX_DATE) {
      return res.status(400).json({ error: "Invalid 'to' date" });
    }
  }

  const where = [];
  const params = [];

  if (from) {
    where.push("date >= ?");
    params.push(from);
  }

  if (to) {
    where.push("date <= ?");
    params.push(to);
  }

  const whereSql = where.length ? `WHERE ${where.join(" AND ")}` : "";

  const sql = `
    SELECT id, temperature, humidity, led_temp, date
    FROM sensors
    ${whereSql}
    ORDER BY datetime(date) ASC
    LIMIT ?
  `;

  params.push(limit);

  db.all(sql, params, (err, rows) => {
    if (err) return res.status(500).json({ error: err.message });
    res.json(rows);
  });
});

app.post("/api/sensors", (req, res) => {
  console.log("Received body:", req.body);
  const { led_temp, temperature, humidity } = req.body;

  if (
    led_temp === undefined ||
    temperature === undefined ||
    humidity === undefined
  ) {
    return res
      .status(400)
      .json({ error: "Temperature and humidity are required" });
  }

  const t = Number(temperature);
  const h = Number(humidity);
  const lt = Number(led_temp);

  if (Number.isNaN(t) || Number.isNaN(h) || Number.isNaN(lt)) {
    return res
      .status(400)
      .json({ error: "Temperature and humidity must be numbers" });
  }

  const sql =
    "INSERT INTO sensors (temperature, humidity, led_temp) VALUES (?, ?, ?)";

  db.run(sql, [t, h, lt], function (err) {
    if (err) return res.status(500).json({ error: err.message });

    res.status(201).json({
      status: "ok",
      commands: commands, // return commands to the client
    });
  });
});

app.post("/api/command", (req, res) => {
  const newCmd = req.body;

  // Validate fan_limits command
  if (newCmd.type === "fan_limits") {
    const minRpm = parseInt(newCmd.min_rpm);
    const maxRpm = parseInt(newCmd.max_rpm);

    // Check if values are valid numbers
    if (isNaN(minRpm) || isNaN(maxRpm)) {
      return res.status(400).json({ error: "RPM values must be numbers" });
    }

    // Check range (0-5000 as in your HTML sliders)
    if (minRpm < 0 || minRpm > 5000 || maxRpm < 0 || maxRpm > 5000) {
      return res
        .status(400)
        .json({ error: "RPM values must be between 0 and 5000" });
    }

    // Check that min < max
    if (minRpm >= maxRpm) {
      return res
        .status(400)
        .json({ error: "min_rpm must be less than max_rpm" });
    }

    // Update command with validated values
    newCmd.min_rpm = minRpm;
    newCmd.max_rpm = maxRpm;

    // Remove any old fan_limits
    commands = commands.filter((cmd) => cmd.type !== "fan_limits");
  }

  commands.push(newCmd);
  console.log("Command added:", newCmd);
  res.json({ ok: true });
});

app.listen(port, "0.0.0.0", () => {
  console.log(`Server running at http://localhost:${port}`);
});
