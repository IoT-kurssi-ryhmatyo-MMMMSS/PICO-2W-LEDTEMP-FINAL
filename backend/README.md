# Sensor Data Monitoring System - Backend

Backend server for collecting and visualizing sensor data (temperature, humidity, LED temperature) with fan control functionality.

## Features

- RESTful API for sensor data storage and retrieval
- Fan control commands (RPM limits)
- Time-range filtering for historical data
- SQLite database for data persistence
- Static file serving for web interface
- Auto-refresh dashboard with real-time charts

## Project Structure
```
project/
├── public/
│   └── index.html          # Web interface
├── index.js                # Main backend server
├── mydb.sqlite3            # SQLite database (auto-created)
├── package.json
└── README.md
```

## Prerequisites

- Node.js 14.x or higher
- npm (Node package manager)

## Installation

1. Clone the repository:
```bash
git clone https://github.com/your-username/your-repository.git
cd your-repository
```

2. Install dependencies:
```bash
npm install
```

This will install:
- `express` (v5.1.0) - Web framework
- `sqlite3` (v5.1.7) - SQLite database driver
- `@types/node` (v22.13.11) - TypeScript type definitions for Node.js

## Configuration

The server runs on port `3000` by default. You can change it using the `PORT` environment variable:
```bash
PORT=8080 node index.js
```

## Running the Server

Start the server:
```bash
node index.js
```

The server will start on `http://localhost:3000`.

Open your browser and navigate to `http://localhost:3000` to access the web interface.

For development with auto-restart, you can install nodemon:
```bash
npm install --save-dev nodemon
```

Then add to your `package.json` scripts:
```json
"scripts": {
  "start": "node index.js",
  "dev": "nodemon index.js"
}
```

And run:
```bash
npm run dev
```

## API Endpoints

### GET /api/sensors
Retrieve sensor data with optional filters.

**Query Parameters:**
- `limit` (optional) - Number of records to return (1-10000, default: 200)
- `from` (optional) - Start date in ISO format (YYYY-MM-DD HH:MM:SS)
- `to` (optional) - End date in ISO format (YYYY-MM-DD HH:MM:SS)

**Example:**
```bash
curl "http://localhost:3000/api/sensors?limit=100&from=2025-01-01%2000:00:00&to=2025-12-31%2023:59:59"
```

**Response:**
```json
[
  {
    "id": 1,
    "temperature": 22.5,
    "humidity": 55.0,
    "led_temp": 23.1,
    "date": "2025-11-30 12:00:00"
  }
]
```

### POST /api/sensors
Add new sensor reading.

**Body:**
```json
{
  "temperature": 22.5,
  "humidity": 55.0,
  "led_temp": 23.1
}
```

**Response:**
```json
{
  "status": "ok",
  "commands": []
}
```

### POST /api/command
Send fan control command.

**Body:**
```json
{
  "type": "fan_limits",
  "min_rpm": 1000,
  "max_rpm": 2500
}
```

**Validation:**
- RPM values must be numbers
- RPM values must be between 0 and 5000
- min_rpm must be less than max_rpm

**Response:**
```json
{
  "ok": true
}
```

### GET /api/sensors/test
Health check endpoint that returns test data.

**Response:**
```json
[
  {
    "id": 1234,
    "temperature": 22.5,
    "humidity": 55,
    "led_temp": 22.6,
    "date": "2025-10-01 15:00:00"
  }
]
```

## Database Schema

The SQLite database (`mydb.sqlite3`) contains a `sensors` table:
```sql
CREATE TABLE sensors (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  temperature REAL,
  humidity REAL,
  led_temp REAL,
  date DATETIME DEFAULT CURRENT_TIMESTAMP
)
```

## Web Interface Features

- **Real-time Visualization** - Google Charts for temperature, humidity, and LED temperature
- **Time Range Selection** - Filter data by custom date ranges (default: last 6 hours)
- **Fan Control Panel** - Adjust minimum and maximum RPM with sliders (800-3000 RPM)
- **Auto-refresh** - Updates every 15 seconds (can be toggled)
- **Data Table** - Detailed view of all sensor readings

## Troubleshooting

### Database not created
Make sure you have write permissions in the project directory. The database file `mydb.sqlite3` will be created automatically on first run.

### Port already in use
Change the port using environment variable:
```bash
PORT=4000 node index.js
```

### CORS issues
The server serves static files from the `public` folder, so no CORS configuration is needed when accessing from the same origin.

### Module not found errors
Make sure you've run `npm install` and that the `"type": "module"` is present in `package.json` (required for ES6 imports).
