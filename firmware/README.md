# Sensor Data Monitoring System - Firmware (Raspberry Pi Pico 2W)

Firmware for Raspberry Pi Pico 2W that collects sensor data (DHT22 temperature/humidity, LED temperature) and sends it to the backend server. Includes LCD display and fan temperature control via server commands.

## Hardware Requirements

- **Raspberry Pi Pico 2W** (RP2350 with WiFi)
- **DHT22** temperature and humidity sensor
- **20x4 I2C LCD Display** (address 0x27)
- **LED temperature sensor** (analog, connected to ADC)
- **Push button** (GPIO16)
- **Breadboard and wiring**

## Pin Configuration
```
DHT22 Sensor:
├── Data Pin → GPIO 15

LCD I2C Display:
├── SDA → GPIO 0
├── SCL → GPIO 1

LED Temperature Sensor:
├── Analog Output → GPIO 26 (ADC0)

Button:
├── Signal → GPIO 16 (with internal pull-up)
```

## Project Structure
```
firmware/
├── src/
│   └── main.cpp           # Main firmware code
├── platformio.ini         # PlatformIO configuration
└── README.md
```

## Features

- **WiFi connectivity** - Connects to your WiFi network
- **Sensor readings** - DHT22 (temp/humidity) + LED temperature sensor
- **LCD display** - Shows real-time sensor data and fan temperature range
- **Push button control** - Toggle data sending ON/OFF
- **Server communication** - Sends data to backend every 5 seconds
- **Fan temperature control** - Receives and displays fan temperature limits from server
- **Noise filtering** - ADC readings filtered with median algorithm
- **NaN protection** - Validates DHT22 readings before sending

## Prerequisites

- [PlatformIO](https://platformio.org/) installed (via VSCode extension or CLI)
- USB cable to connect Pico 2W to your computer

## Installation

### 1. Install PlatformIO

**VSCode:**
- Install the "PlatformIO IDE" extension from the marketplace

**Command Line:**
```bash
pip install platformio
```

### 2. Clone and Configure
```bash
git clone https://github.com/your-username/your-repository.git
cd your-repository/firmware
```

### 3. Configure WiFi and Server

Edit `src/main.cpp` and set your credentials:
```cpp
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const char *serverUrl = "https://your-server.com/api/sensors";
```

**Important:** Keep credentials private! Consider creating `config.h` for sensitive data.

### 4. Build and Upload

**Using PlatformIO CLI:**
```bash
pio run --target upload
```

**Using VSCode:**
- Click "PlatformIO: Upload" in the bottom toolbar
- Or press `Ctrl+Alt+U` (Windows/Linux) or `Cmd+Alt+U` (Mac)

### 5. Monitor Serial Output
```bash
pio device monitor
```

Or use the "Serial Monitor" button in VSCode.

## Dependencies

The project uses the following libraries (auto-installed by PlatformIO):
```ini
adafruit/DHT sensor library          # DHT22 sensor support
adafruit/Adafruit Unified Sensor     # Sensor abstraction layer
marcoschwartz/LiquidCrystal_I2C      # I2C LCD display driver
bblanchon/ArduinoJson @ ^7.0.0       # JSON parsing for server responses
```

## Configuration

### Measurement Interval

Change the measurement and sending interval (default: 5 seconds):
```cpp
const unsigned long MEASURE_INTERVAL_MS = 5000; // milliseconds
```

### LCD I2C Address

If your LCD has a different I2C address, modify:
```cpp
LiquidCrystal_I2C lcd(0x27, 20, 4); // Change 0x27 to your address
```

Find your LCD address with an I2C scanner sketch.

### Fan Temperature Defaults

Initial fan temperature limits (before server updates):
```cpp
int fan_min_temp = 20;  // °C
int fan_max_temp = 50;  // °C
```

### LED Temperature Calibration

Adjust the temperature calculation formula if needed:
```cpp
led_temp = -500.0f * voltage + 828.0f;
```

## LCD Display Layout
```
┌──────────────────────┐
│Temp:  22.5 C         │  Line 0: Temperature from DHT22
│ Hum:  55.0 %         │  Line 1: Humidity from DHT22
│ LED:  23.1 C         │  Line 2: LED temperature
│Send:OFF      20-50C  │  Line 3: Send status + Fan temp range
└──────────────────────┘
```

## Usage

### Starting the System

1. Power on the Pico 2W
2. LCD shows "WiFi..." while connecting
3. After connection, displays sensor readings
4. Press button to toggle sending ON/OFF

### Button Control

- **Press once** - Toggle data sending ON/OFF
- **Send:OFF** - Data is collected but NOT sent to server
- **Send: ON** - Data is sent to server every 5 seconds

### Serial Monitor Output
```
T:22.5 H:55.0 LED:23.1 Fan:20-50C
HTTP: 201
Fan limits updated: 25-60C
```

## Server Communication

### POST Request Format

The firmware sends JSON data:
```json
{
  "temperature": 22.5,
  "humidity": 55.0,
  "led_temp": 23.1
}
```

### Server Response Format

The server responds with:
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

## Troubleshooting

### WiFi Connection Failed
- Check SSID and password in code
- Verify 2.4GHz network (Pico 2W doesn't support 5GHz)
- Move closer to router

### LCD Not Working
- Check I2C address (try 0x3F if 0x27 doesn't work)
- Verify SDA/SCL connections (GPIO 0/1)
- Check power supply (3.3V or 5V depending on LCD module)

### DHT22 Reading NaN
- Check DHT22 wiring (VCC, GND, Data to GPIO15)
- Add 10kΩ pull-up resistor between Data and VCC
- Wait 2 seconds after power-on for sensor to stabilize
- Firmware will skip sending if NaN detected

### Server Communication Fails
- Verify server URL is correct (remove port if using Replit)
- Check server is running and accessible
- Test with curl from another device
- Verify SSL/HTTPS certificate (using `setInsecure()`)

### Upload Fails
- Hold BOOTSEL button while connecting USB
- Check USB cable supports data transfer
- Try different USB port
- Ensure no other program is using the serial port

## Platform Information

- **Platform**: Raspberry Pi Pico SDK (via PlatformIO)
- **Board**: Raspberry Pi Pico 2W (RP2350)
- **Framework**: Arduino
- **Core**: Earle Philhower Arduino-Pico core
- **Baud Rate**: 115200