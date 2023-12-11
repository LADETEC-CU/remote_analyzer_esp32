#include "secrets.h"
#include "ArduinoJson.h"
#include "configs.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <random>

// Define the URL for the API endpoint
const char *url = "https://www.ladetec.com/demo/api/sensor";

// Function to generate phase data
DynamicJsonDocument generatePhaseData()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> voltageDist(108, 120);
  std::uniform_real_distribution<float> currentDist(0, 25);
  std::uniform_real_distribution<float> pfDist(0.6, 1);

  float voltage = voltageDist(gen);
  float current = currentDist(gen);
  float pf = pfDist(gen);
  float pa = voltage * current;
  float power = pa * pf;
  float pr = sqrt(pow(pa, 2) - pow(power, 2));

  DynamicJsonDocument json(128);
  json["voltage"] = voltage;
  json["current"] = current;
  json["power"] = power;
  json["power_r"] = pr;
  json["power_a"] = pa;
  json["power_factor"] = pf;

  return json;
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  DEBUG_PRINTLN(WiFi.localIP());
}

void setup()
{
  if (DEBUG)
  {
    Serial.begin(115200);
  }
  // Connect to WiFi
  initWiFi();
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // Create a data payload in JSON format
    DynamicJsonDocument data(512);
    data["phase1"] = generatePhaseData();
    data["phase2"] = generatePhaseData();
    data["phase3"] = generatePhaseData();

    // Convert JSON to string
    String jsonString;
    serializeJson(data, jsonString);

    // Send the data to the API endpoint using the POST method
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonString);
    if (httpResponseCode == 200)
    {
      // The data was successfully uploaded
      DEBUG_PRINTLN("Data uploaded successfully");
      DynamicJsonDocument response(512);
      deserializeJson(response, http.getString());
      DEBUG_PRINTLN(response["digital_outputs"].as<String>());
    }
    else
    {
      // An error occurred
      Serial.printf("An error occurred: %d\n", httpResponseCode);
    }

    http.end();
  }

  delay(5000);
}