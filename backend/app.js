import express from "express";
import sqlite3 from "sqlite3";
import path from "path";
import { fileURLToPath } from "url";
import morgan from "morgan";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();

// Add custom token for logging request body
morgan.token("body", (req) => JSON.stringify(req.body));

// Use morgan for logging unless in test mode
if (process.env.NODE_ENV !== "test") {
  app.use(
    morgan(
      ":method :url :status :res[content-length] - :response-time ms :body"
    )
  );
}

app.use(express.json());

// Serve static frontend from ./public
app.use(express.static(path.join(__dirname, "public")));
let commands = [];

var db;
if (process.env.NODE_ENV === "test") {
  db = new sqlite3.Database(":memory:", (err) => {
    if (err) return console.error(err.message);
    console.log("Connected to in-memory SQLite database for testing");
  });
} else {
  db = new sqlite3.Database("./mydb.sqlite3", (err) => {
    if (err) return console.error(err.message);
    console.log("Connected to SQLite database");
  });
}

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

if (process.env.NODE_ENV === "test") {
  app.post("/api/sensors/reset", (req, res) => {
    db.run("DELETE FROM sensors", (err) => {
      if (err) return res.status(500).json({ error: err.message });
      commands = [];
      res.json({ status: "ok" });
    });
  });
}

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

  // Apply limit only if NOT both dates are specified
  const hasFullRange = from && to;
  const limitSql = hasFullRange ? "" : "LIMIT ?";

  const sql = `
    SELECT id, temperature, humidity, led_temp, date
    FROM sensors
    ${whereSql}
    ORDER BY datetime(date) DESC
    ${limitSql}
  `;

  if (!hasFullRange) {
    params.push(limit);
  }

  db.all(sql, params, (err, rows) => {
    if (err) return res.status(500).json({ error: err.message });
    res.json(rows);
  });
});

app.post("/api/sensors", (req, res) => {
  //console.log("Received body:", req.body);
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

  if (newCmd.type === "fan_limits") {
    const minTemp = parseInt(newCmd.min_temp);
    const maxTemp = parseInt(newCmd.max_temp);

    if (isNaN(minTemp) || isNaN(maxTemp)) {
      return res
        .status(400)
        .json({ error: "Temperature values must be numbers" });
    }

    if (minTemp < 0 || minTemp > 100 || maxTemp < 0 || maxTemp > 100) {
      return res
        .status(400)
        .json({ error: "Temperature values must be between 0 and 100°C" });
    }

    if (minTemp >= maxTemp) {
      return res
        .status(400)
        .json({ error: "min_temp must be less than max_temp" });
    }

    newCmd.min_temp = minTemp;
    newCmd.max_temp = maxTemp;

    commands = commands.filter((cmd) => cmd.type !== "fan_limits");
  }

  commands.push(newCmd);
  if (process.env.NODE_ENV !== "test") {
    console.log("Command added:", newCmd);
  }
  res.json({ ok: true });
});

export { app };
