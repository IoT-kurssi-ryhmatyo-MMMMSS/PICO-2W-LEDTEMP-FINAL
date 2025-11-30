# Sensor Data Monitoring System - Backend

Backend server for collecting and visualizing sensor data (temperature, humidity, LED temperature) with fan temperature control functionality.

## Features

- RESTful API for sensor data storage and retrieval
- Fan temperature control commands (temperature range)
- Time-range filtering for historical data
- SQLite database for data persistence
- Static file serving for web interface
- Auto-refresh dashboard with real-time charts
- Dual range slider (20-80°C) for fan control

## Project Structure
```
backend/
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
cd your-repository/backend
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
  - **Note:** `limit` is disabled when date range (`from`/`to`) is specified
- `from` (optional) - Start date in ISO format (YYYY-MM-DD HH:MM:SS)
- `to` (optional) - End date in ISO format (YYYY-MM-DD HH:MM:SS)

**Example:**
```bash
# Get last 200 records (default)
curl "http://localhost:3000/api/sensors"

# Get specific date range
curl "http://localhost:3000/api/sensors?from=2025-01-01%2000:00:00&to=2025-12-31%2023:59:59"

# Get last 100 records
curl "http://localhost:3000/api/sensors?limit=100"
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
  "commands": [
    {
      "type": "fan_limits",
      "min_temp": 20,
      "max_temp": 50
    }
  ]
}
```

### POST /api/command
Send fan temperature control command.

**Body:**
```json
{
  "type": "fan_limits",
  "min_temp": 20,
  "max_temp": 50
}
```

**Validation:**
- Temperature values must be integers
- Temperature values must be between 0 and 100°C
- min_temp must be less than max_temp

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
- **Time Range Selection** - Filter data by custom date ranges (empty by default to show last 200 records)
- **Intelligent Limit Control** - Limit field disabled when date range is selected
- **Fan Temperature Control** - Dual range slider (20-80°C) with visual feedback
- **Auto-refresh** - Updates every 15 seconds (can be toggled)
- **Data Table** - Detailed view of all sensor readings with local timezone

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

### No data showing after refresh
Ensure the date range fields (`From`/`To`) are empty to show all data. If set, only data within that range will display.