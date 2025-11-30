#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <algorithm>
#include <numeric>
#include <ArduinoJson.h>

const char* ssid = "FRITZ!Box 6690 UA";
const char* password = "31135659524764098463";
const char* serverUrl = "https://fbf04876-1d7e-4412-be0f-123ef6bf30a3-00-1d0pb9ugzfrze.spock.replit.dev:3000/api/sensors";

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 20, 4);

// Button on GPIO16 (polled, no interrupts)
const int buttonPin = 16;
bool sendEnabled = false;
bool prevSendEnabled = false;

bool buttonState = HIGH;
bool lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // ms

float temperature = 0;
float humidity = 0;
float led_temp = 0;

// Fan limits from server (and initial hardcoded values)
int fan_min_temp = 20;
int fan_max_temp = 50;

// Measurement/send interval
const unsigned long MEASURE_INTERVAL_MS = 5000;
unsigned long lastMeasureTime = 0;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // ADC 12-bit resolution
    analogReadResolution(12);

    // DHT
    dht.begin();

    // LCD
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.print("WiFi...");

    // Button
    pinMode(buttonPin, INPUT_PULLUP);

    // WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nOK!");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.print("Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp:       ");
    lcd.setCursor(0, 1);
    lcd.print(" Hum:        ");
    lcd.setCursor(0, 2);
    lcd.print(" LED:        ");
    lcd.setCursor(0, 3);
    lcd.print("Send:OFF");
}

void loop()
{
    unsigned long now = millis();

    // ----- Button polling with debounce -----
    int reading = digitalRead(buttonPin);

    if (reading != lastButtonReading)
    {
        lastDebounceTime = now;
    }

    if ((now - lastDebounceTime) > debounceDelay)
    {
        if (reading != buttonState)
        {
            buttonState = reading;
            if (buttonState == LOW)
            {                               // button pressed
                sendEnabled = !sendEnabled; // toggle send mode
            }
        }
    }

    lastButtonReading = reading;

    // Update "Send: ON/OFF" when flag changes
    if (sendEnabled != prevSendEnabled)
    {
        lcd.setCursor(0, 3);
        if (sendEnabled)
        {
            lcd.print("Send: ON ");
        }
        else
        {
            lcd.print("Send:OFF");
        }
        prevSendEnabled = sendEnabled;
    }

    // ----- Periodic measurement and sending (every MEASURE_INTERVAL_MS) -----
    if (now - lastMeasureTime >= MEASURE_INTERVAL_MS)
    {
        lastMeasureTime = now;

        // Read DHT
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();

        // Read ADC for LED temp with noise filtering
        int adc_arr[30];
        for (int i = 0; i < 30; i++)
        {
            adc_arr[i] = analogRead(26);
        }

        std::sort(adc_arr, adc_arr + 30);
        long adc_sum = std::accumulate(adc_arr + 10, adc_arr + 20, 0L);

        float voltage = (adc_sum / 40960.0f) * 3.31f;
        led_temp = -500.0f * voltage + 828.0f;

        // LCD update
        lcd.setCursor(6, 0);
        lcd.print(temperature, 1);
        lcd.print(" C  ");

        lcd.setCursor(6, 1);
        lcd.print(humidity, 1);
        lcd.print(" %  ");

        lcd.setCursor(6, 2);
        lcd.print(led_temp, 1);
        lcd.print(" C  ");

        // Fan RPM range on the right part of line 3
        lcd.setCursor(10, 3);
        char fanBuf[16];
        snprintf(fanBuf, sizeof(fanBuf), "%d-%dC", fan_min_temp, fan_max_temp);
        lcd.print("          ");
        lcd.setCursor(10, 3);
        lcd.print(fanBuf);

        Serial.print("T:");
        Serial.print(temperature);
        Serial.print(" H:");
        Serial.print(humidity);
        Serial.print(" LED:");
        Serial.print(led_temp);
        Serial.print(" Fan:");
        Serial.print(fan_min_temp);
        Serial.print("-");
        Serial.print(fan_max_temp);
        Serial.println("C");

        // Send to server ONLY when sendEnabled is true
        if (sendEnabled && WiFi.status() == WL_CONNECTED)
        {
            WiFiClientSecure client;
            client.setInsecure();

            HTTPClient http;
            http.begin(client, serverUrl);
            http.addHeader("Content-Type", "application/json");

            String payload = "{\"temperature\":" + String(temperature) +
                             ",\"humidity\":" + String(humidity) +
                             ",\"led_temp\":" + String(led_temp) + "}";

            // Serial.print("Payload: ");
            // Serial.println(payload);

            int code = http.POST(payload);
            Serial.print("HTTP: ");
            Serial.println(code);

            if (code == 200 || code == 201)
            {
                // Read JSON response and update fan limits if commands are present
                String resp = http.getString();

                DynamicJsonDocument doc(1024);
                DeserializationError err = deserializeJson(doc, resp);

                if (!err)
                {
                    if (doc["commands"].is<JsonArray>())
                    {
                        JsonArray commands = doc["commands"].as<JsonArray>();
                        for (JsonObject cmd : commands)
                        {
                            const char *type = cmd["type"] | "";
                            if (strcmp(type, "fan_limits") == 0)
                            {
                                int min_temp = cmd["min_temp"] | fan_min_temp;
                                int max_temp = cmd["max_temp"] | fan_max_temp;

                                if (min_temp != fan_min_temp || max_temp != fan_max_temp)
                                {
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
                else
                {
                    Serial.print("JSON error: ");
                    Serial.println(err.c_str());
                }
            }

            http.end();
        }
    }
}
