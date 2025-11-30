# PICO-2W-LEDTEMP-FINAL

IoT project for embedded systems course. Raspberry Pi Pico 2 W collects sensor data (DHT22, ADC) and sends it via HTTPS to Node.js server. Web interface displays real-time data with charts.

**Tech stack:** C++ (Arduino), Node.js, Express, SQLite, Google Charts

## Project Structure
```
PICO-2W-LEDTEMP-FINAL/
├── README.md                   # This file
├── backend/                    # Backend server
│   ├── README.md              # Backend documentation
│   ├── index.js
│   ├── package.json
│   └── public/
│       └── index.html
└── firmware/                   # Firmware for Pico 2W
    ├── README.md              # Firmware documentation
    ├── platformio.ini
    └── src/
        └── main.cpp
```

## Components

### 📡 [Firmware](./firmware/README.md)
Raspberry Pi Pico 2W firmware that collects sensor data and communicates with the backend server.

**Features:**
- DHT22 temperature and humidity sensor
- LED temperature sensor (ADC)
- 20x4 I2C LCD display
- Push button control for sending data
- WiFi connectivity
- Fan control via server commands

[📖 Read firmware documentation →](./firmware/README.md)

### 🖥️ [Backend](./backend/README.md)
Node.js server with Express and SQLite database for data storage and web interface.

**Features:**
- RESTful API for sensor data
- SQLite database storage
- Fan control commands
- Real-time data visualization
- Web dashboard with charts

[📖 Read backend documentation →](./backend/README.md)

## Quick Start

### 1. Setup Backend
```bash
cd backend
npm install
node index.js
```

Server will run on `http://localhost:3000`

### 2. Setup Firmware
```bash
cd firmware
# Configure WiFi and server URL in src/main.cpp
pio run --target upload
```

### 3. Access Web Interface

Open browser: `http://localhost:3000`

## System Architecture
```
┌─────────────────────┐
│  Raspberry Pi Pico  │
│       2W            │
│  ┌───────────────┐  │
│  │  DHT22 Sensor │  │
│  │  LED Temp     │  │      HTTPS POST (sensor data)
│  │  LCD Display  │  ├──────────────────────────────┐
│  │  Button       │  │                              │
│  │  Fan RPM      │◄─┤◄─────────────────────────────┤
│  └───────────────┘  │  Response (commands array)   │
└─────────────────────┘                              ▼
                                            ┌─────────────────┐
                                            │  Node.js Server │
                                            │  ┌────────────┐ │
         ┌──────────────────────────────────┤  │  Express   │ │
         │         HTTP GET/POST            │  │  SQLite    │ │
         │                                  │  │  API       │ │
         │                                  │  └────────────┘ │
         │                                  └─────────────────┘
         │
         ▼
┌─────────────────┐
│  Web Interface  │
│  Google Charts  │
│  Fan Control    │◄─── User sets fan limits (min/max RPM)
└─────────────────┘
         │
         │ POST /api/command
         └─────────────────────► Commands stored in server
                                  (sent to Pico on next POST)
```

**Communication Flow:**

1. **Pico → Server:** POST `/api/sensors` with sensor data
```json
   { "temperature": 22.5, "humidity": 55.0, "led_temp": 23.1 }
```

2. **Server → Pico:** Response with commands array
```json
   {
     "status": "ok",
     "commands": [
       { "type": "fan_limits", "min_rpm": 1200, "max_rpm": 2800 }
     ]
   }
```

3. **Web → Server:** POST `/api/command` to set fan limits
```json
   { "type": "fan_limits", "min_rpm": 1200, "max_rpm": 2800 }
```

4. **Pico updates:** Fan RPM range displayed on LCD

## Hardware Requirements

- Raspberry Pi Pico 2W (RP2350)
- DHT22 temperature/humidity sensor
- 20x4 I2C LCD display
- LED temperature sensor (analog)
- Push button
- Breadboard and wiring

## Software Requirements

**Backend:**
- Node.js 14.x or higher
- npm

**Firmware:**
- PlatformIO (VSCode extension or CLI)
- USB cable for programming

## Features

- ✅ Real-time sensor data collection
- ✅ Temperature and humidity monitoring
- ✅ LED temperature measurement
- ✅ WiFi connectivity
- ✅ HTTPS communication
- ✅ SQLite database storage
- ✅ Web dashboard with charts
- ✅ Historical data viewing
- ✅ Fan control (RPM limits)
- ✅ Auto-refresh (15s interval)
- ✅ LCD display for local monitoring

## API Endpoints

- `GET /api/sensors` - Retrieve sensor data
- `POST /api/sensors` - Add new sensor reading
- `POST /api/command` - Send fan control command
- `GET /api/sensors/test` - Health check endpoint

For detailed API documentation, see [Backend README](./backend/README.md).

## Course Information

Embedded Systems Course Project
LAB University of Applied Sciences
2025