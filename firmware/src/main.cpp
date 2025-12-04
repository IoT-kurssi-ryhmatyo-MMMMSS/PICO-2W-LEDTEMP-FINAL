#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

extern "C" {
#include "hardware/gpio.h"
#include "hardware/pwm.h"
}

// See config.example.h
#include "config.h"

#define DHT_PIN 15
#define DHT_TYPE DHT22
#define LED_ADC_PIN 28
#define FAN_PWM_PIN 16
#define BUTTON_PIN 19
#define USE_OLED 1

DHT dht(DHT_PIN, DHT_TYPE);
#ifdef USE_OLED
    #include <oled.h>
    OLED display(0, 1, NO_RESET_PIN, OLED::W_128, OLED::H_64, OLED::CTRL_SH1106, 0x3C);
#else
    #include <LiquidCrystal_I2C.h>
    #include <Wire.h>
    LiquidCrystal_I2C display(0x27, 20, 4);
#endif

// LED calibration constants
constexpr float LED_REF_VOLTAGE = 3.3f;
constexpr uint16_t LED_ADC_MAX = (1u << 12) - 1;
constexpr float LED_CALIBRATION_TEMP = 24.6f;
constexpr float LED_CALIBRATION_VOLTAGE = 1.7221454f;
constexpr float LED_COEFFICIENT = -0.001876f;

// Measurement/send interval
constexpr unsigned long MEASURE_INTERVAL_MS = 5000;
unsigned long lastMeasureTime = 0;
constexpr int N_SAMPLES = 200;
constexpr int MEASUREMENT_COUNT = 3;

// Fan control constants
constexpr int FAN_MIN_SPEED = 20;
int fan_min_temp = 20; // Fan limits from server (and initial hardcoded values)
int fan_max_temp = 50;
// 1000 steps, 150 mHZ / (999+1)*7.5 = 20kHz
constexpr uint16_t FAN_PWM_WRAP = 999;
constexpr float FAN_PWM_CLKDIV = 7.5f;

// Sensor and button state variables
float temperature = 0;
float humidity = 0;
float led_temp = 0;
float led_voltage = 0;
bool dht_error = false;

// Button interrupt and debounce
volatile bool buttonInterruptFlag = false;
bool sendEnabled = false;
bool prevSendEnabled = false;
unsigned long lastDebounceTime = 0;
constexpr unsigned long debounceDelay = 1000; // ms

// Interrupt Service Routine for button
void handleButtonInterrupt() {
    buttonInterruptFlag = true;
}

void set_speed(uint8_t percent) {
    uint16_t duty = (uint16_t)(percent / 100.0f * FAN_PWM_WRAP);
    pwm_set_gpio_level(FAN_PWM_PIN, duty);
}

void readSensors() {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    dht_error = isnan(temperature) || isnan(humidity);
    if (dht_error) {
        Serial.println("DHT sensor error: NAN value detected. Attempting to recover...");
        dht.begin(); // Try to reinitialize DHT
    }
}

void measureLedTemp() {
    float led_temperature_sum = 0.0f;
    float led_voltage_sum = 0.0f;
    for (int i = 0; i < MEASUREMENT_COUNT; i++) {
        if (buttonInterruptFlag)
            return;
        delay(1000);
        uint32_t voltage_sum = 0;
        for (int j = 0; j < N_SAMPLES; j++) {
            if (buttonInterruptFlag)
                return;
            voltage_sum += analogRead(LED_ADC_PIN);
            delayMicroseconds(50);
        }
        float led_raw = (float)voltage_sum / (float)N_SAMPLES;
        float led_voltage = (led_raw / (float)LED_ADC_MAX) * LED_REF_VOLTAGE;
        led_voltage_sum += led_voltage;
        led_temperature_sum += LED_CALIBRATION_TEMP + (led_voltage - LED_CALIBRATION_VOLTAGE) / LED_COEFFICIENT;
    }
    float led_voltage_average = led_voltage_sum / MEASUREMENT_COUNT;
    led_voltage = led_voltage_average;
    led_temp = led_temperature_sum / MEASUREMENT_COUNT;
}

void updateDisplay(uint8_t fan_speed) {
#ifdef USE_OLED
    char buf[16];
    display.draw_rectangle(40, 0, 127, 49, OLED::SOLID, OLED::BLACK);
    display.draw_rectangle(85, 50, 127, 60, OLED::SOLID, OLED::BLACK);
    if (dht_error) {
        display.draw_string(50, 0, "DHT ERR");
    }
    else {
        sprintf(buf, "%.1f C", temperature);
        display.draw_string(50, 0, buf);
        sprintf(buf, "%.1f %%", humidity);
        display.draw_string(50, 10, buf);
    }
    sprintf(buf, "%.1f C", led_temp);
    display.draw_string(50, 20, buf);
    sprintf(buf, "%d.0 %%", fan_speed);
    display.draw_string(50, 30, buf);
    sprintf(buf, "%.4f V", led_voltage);
    display.draw_string(40, 40, buf);
    sprintf(buf, "%d-%dC", fan_min_temp, fan_max_temp);
    display.draw_string(85, 50, buf);
    

    display.display();
#else
    char buf[16];
    display.setCursor(6, 0);
    if (dht_error) {
        display.print("DHT ERR");
    }
    else {
        display.print(temperature, 1);
        display.print(" C  ");
        display.setCursor(6, 1);
        display.print(humidity, 1);
        display.print(" %  ");
    }
    display.setCursor(6, 2);
    display.print(led_temp, 1);
    display.print(" C  ");
    display.setCursor(10, 3);
    display.print("          ");
    display.setCursor(10, 3);
    snprintf(buf, sizeof(buf), "%d-%dC", fan_min_temp, fan_max_temp);
    display.print(buf);
#endif
}

void sendData(uint8_t fan_speed) {
    if (sendEnabled && WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Trying to reconnect to WiFi...");
        WiFi.begin(ssid, password);
        for(int i = 0; i < 20; i++) {
            delay(500);
            Serial.print(".");
        }
    }
    if (sendEnabled && WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.begin(client, serverUrl);
        http.addHeader("Content-Type", "application/json");
        String payload = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) +
                         ",\"led_temp\":" + String(led_temp) + "}";
        int code = http.POST(payload);
        Serial.print("HTTP: ");
        Serial.println(code);
        if (code == 200 || code == 201) {
            String resp = http.getString();
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, resp);
            if (!err) {
                if (doc["commands"].is<JsonArray>()) {
                    JsonArray commands = doc["commands"].as<JsonArray>();
                    for (JsonObject cmd : commands) {
                        const char* type = cmd["type"] | "";
                        if (strcmp(type, "fan_limits") == 0) {
                            int min_temp = cmd["min_temp"] | fan_min_temp;
                            int max_temp = cmd["max_temp"] | fan_max_temp;
                            if (min_temp != fan_min_temp || max_temp != fan_max_temp) {
                                fan_min_temp = min_temp;
                                fan_max_temp = max_temp;
                                Serial.print("Fan limits updated: ");
                                Serial.print(fan_min_temp);
                                Serial.print("-");
                                Serial.print(fan_max_temp);
                                Serial.println("C");
                            }
                        }
                    }
                }
            }
            else {
                Serial.print("JSON error: ");
                Serial.println(err.c_str());
            }
        }
        else {
            Serial.print("HTTP error code: ");
            Serial.println(code);
            Serial.print("HTTP error response: ");
            Serial.println(http.getString());
        }
        http.end();
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    // ADC 12-bit resolution
    analogReadResolution(12);

    // DHT
    dht.begin();

// LCD/OLED
#ifdef USE_OLED
    display.begin();
    display.useOffset();
    display.drawString(5, 5, "WiFi...");
    #ifdef USE_SHARK
        display.clear();
        display.draw_bitmap(0, 0, 128, 64, shark);
    #endif
    display.display();
#else
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();
    display.init();
    display.backlight();
    display.clear();
    display.print("WiFi...");
#endif

    // Button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(BUTTON_PIN, handleButtonInterrupt, FALLING);

    // WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nOK!");
    Serial.println(WiFi.localIP());

// LCD/OLED update
#ifdef USE_OLED
    display.clear();
    display.drawString(10, 10, "Connected!");
    String ipStr = WiFi.localIP().toString();
    display.drawString(10, 30, ipStr.c_str());
    display.display();
    delay(2000);
    display.clear();
    display.setCursor(0, 0);
    display.print("Temp:");
    display.setCursor(0, 10);
    display.print("Hum:");
    display.setCursor(0, 20);
    display.print("LED:");
    display.setCursor(0, 30);
    display.print("Fan:");
    display.setCursor(0, 40);
    display.print("V:");
    display.setCursor(0, 50);
    display.print("Send:OFF");
#else
    display.clear();
    display.print("Connected!");
    display.setCursor(0, 1);
    display.print(WiFi.localIP());
    delay(2000);
    display.clear();
    display.setCursor(0, 0);
    display.print("Temp:       ");
    display.setCursor(0, 1);
    display.print(" Hum:        ");
    display.setCursor(0, 2);
    display.print(" LED:        ");
    display.setCursor(0, 3);
    display.print("Send:OFF");
#endif
    Serial.println("Setup complete");

    // Fan PWM setup
    gpio_set_function(FAN_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(FAN_PWM_PIN);
    pwm_set_wrap(slice_num, FAN_PWM_WRAP);
    pwm_set_clkdiv(slice_num, FAN_PWM_CLKDIV);
    set_speed(FAN_MIN_SPEED);
    pwm_set_enabled(slice_num, true);
}

void loop() {
    unsigned long now = millis();

    // Button interrupt
    if (buttonInterruptFlag) {
        buttonInterruptFlag = false;
        unsigned long now = millis();
        int reading = digitalRead(BUTTON_PIN);
        if (reading == LOW && (now - lastDebounceTime) > debounceDelay) {
            lastDebounceTime = now;
            sendEnabled = !sendEnabled; // toggle send mode
        }
    }

    // Update "Send: ON/OFF" when flag changes
    if (sendEnabled != prevSendEnabled) {
    #ifdef USE_OLED
        display.setCursor(0, 50);
        display.draw_rectangle(0, 50, 60, 60, OLED::SOLID, OLED::BLACK);
        display.print(sendEnabled ? "Send: ON" : "Send:OFF");
        display.display();
    #else
        display.setCursor(0, 3);
        display.print(sendEnabled ? "Send: ON " : "Send:OFF");
    #endif
        Serial.println(sendEnabled ? "Data sending ENABLED" : "Data sending DISABLED");
        prevSendEnabled = sendEnabled;
    }

    // Measure every MEASURE_INTERVAL_MS
    if (now - lastMeasureTime >= MEASURE_INTERVAL_MS) {
        lastMeasureTime = now;

        // Read sensors and handle errors
        readSensors();
        measureLedTemp();

        // Adjust fan speed based on LED temperature
        uint8_t fan_speed;
        if (led_temp <= fan_min_temp) {
            fan_speed = FAN_MIN_SPEED;
        }
        else if (led_temp >= fan_max_temp) {
            fan_speed = 100;
        }
        else {
            fan_speed = FAN_MIN_SPEED + ((led_temp - fan_min_temp) * (100 - FAN_MIN_SPEED) / (fan_max_temp - fan_min_temp));
        }
        set_speed(fan_speed);

        updateDisplay(fan_speed);

        Serial.print("T:");
        Serial.print(temperature);
        Serial.print(" H:");
        Serial.print(humidity);
        Serial.print(" LED:");
        Serial.print(led_temp);
        Serial.print(" Fan:");
        Serial.print(fan_speed);
        Serial.print("% V:");
        Serial.print(led_voltage, 4);
        Serial.print(" Range: ");
        Serial.print(fan_min_temp);
        Serial.print("-");
        Serial.print(fan_max_temp);
        Serial.println("C");

        if (!buttonInterruptFlag) {
            sendData(fan_speed);
        } else {
            Serial.println("Skipping sendData due to interrupt.");
        }
    }
}
