#include "secrets.h"
#include "ArduinoJson.h"
#include "configs.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <random>
// github link: https://github.com/4-20ma/ModbusMaster
#include <ModbusMaster.h>

// Define the URL for the API endpoint
const char *url = "https://www.ladetec.com/demo/api/sensor";

// Initialize the ModbusMaster object as node
ModbusMaster node;

// Pin 4 made high for Modbus transmision mode
void modbusPreTransmission()
{
  delay(50);
  digitalWrite(MODBUS_DIR_PIN, HIGH);
}
// Pin 4 made low for Modbus receive mode
void modbusPostTransmission()
{
  digitalWrite(MODBUS_DIR_PIN, LOW);
  delay(50);
}

// Function to generate phase data
DynamicJsonDocument generatePhaseData(float voltage,
                                      float current,
                                      float pf,
                                      float pa,
                                      float power,
                                      float pr)
{
  DynamicJsonDocument json(128);
  json["voltage"] = voltage;
  json["current"] = current;
  json["power"] = power;
  json["power_r"] = pr;
  json["power_a"] = pa;
  json["power_factor"] = pf;

  return json;
}

void setupPins()
{
  // Digital outputs
  pinMode(PIN_LUZ, OUTPUT);
  pinMode(PIN_COCINA, OUTPUT);
  digitalWrite(PIN_LUZ, HIGH);
  digitalWrite(PIN_COCINA, HIGH);
}

void setupModbus()
{
  // RS-485 control pin
  pinMode(MODBUS_DIR_PIN, OUTPUT);
  digitalWrite(MODBUS_DIR_PIN, LOW);

  // Serial2.begin(baud-rate, protocol, RX pin, TX pin);.
  Serial2.begin(MODBUS_SERIAL_BAUD, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  Serial2.setTimeout(1000);
  node.begin(SLAVE_ID, Serial2);

  //  callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(modbusPreTransmission);
  node.postTransmission(modbusPostTransmission);
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DEBUG_PRINTLN("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    DEBUG_PRINTLN('.');
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
  // Configure digital outputs
  setupPins();
  // Connect to WiFi
  initWiFi();
  // Configure modbus master
  setupModbus();
}

int pfFilter(int pf)
{
  bool neg = pf >> 7;
  pf = pf & 0x7f;
  if (neg)
  {
    pf *= -1;
  }
  return pf;
}

void loop()
{
  uint8_t result;
  uint16_t data[48];
  int i;
  for (i = 0; i <= 4; i++)
  {
    // Modbus function 0x03 Read Holding Registers according to energy meter datasheet
    result = node.readHoldingRegisters(START_ADDRESS + 2 * NUM_WORDS * i, NUM_WORDS);
    if (result == node.ku8MBSuccess)
    {
      DEBUG_PRINTLN("Success, Received data: ");

      // Retrieve the data from getResponseBuffer(uint8_t u8Index) function.
      for (int k = 0; k < NUM_WORDS; k++)
      {
        data[k + NUM_WORDS * i] = node.getResponseBuffer(k);
        char value_str[10];
        // String(data[k + NUM_WORDS * i]).toCharArray(value_str, 10);
        // DEBUG_PRINT(value_str);
        // DEBUG_PRINT(", ");
      }
      // DEBUG_PRINTLN();
    }
    else
    {
      DEBUG_PRINT("Failed, Response Code: ");
      DEBUG_PRINTLN(result);
      delay(5000);
    }
  }
  // Parse data
  float VL1_N = data[0] / 10;
  float AL1 = data[1] * TC_RATIO / 1000;
  float WL1 = data[2] * TC_RATIO / 10;

  float VL2_N = data[3] / 10;
  float AL2 = data[4] * TC_RATIO / 1000;
  float WL2 = data[5] * TC_RATIO / 10;

  float VL3_N = data[6] / 10;
  float AL3 = data[7] * TC_RATIO / 1000;
  float WL3 = data[8] * TC_RATIO / 10;

  float VA_L1 = data[16] * TC_RATIO / 10;
  float VA_L2 = data[17] * TC_RATIO / 10;
  float VA_L3 = data[18] * TC_RATIO / 10;

  float VAR_L1 = data[20] * TC_RATIO / 10;
  float VAR_L2 = data[21] * TC_RATIO / 10;
  float VAR_L3 = data[22] * TC_RATIO / 10;

  float PF_L1 = pfFilter(data[30] and 0xff) / 100;
  float PF_L2 = pfFilter(data[30] >> 8) / 100;
  float PF_L3 = pfFilter(data[31] and 0xff) / 100;

  if (WiFi.status() == WL_CONNECTED)
  {
    // Create a data payload in JSON format
    DynamicJsonDocument json_data(512);
    json_data["phase1"] = generatePhaseData(VL1_N, AL1, PF_L1, VA_L1, WL1, VAR_L1);
    json_data["phase2"] = generatePhaseData(VL2_N, AL2, PF_L2, VA_L2, WL2, VAR_L2);
    json_data["phase3"] = generatePhaseData(VL3_N, AL3, PF_L3, VA_L3, WL3, VAR_L3);

    // DEBUG_PRINTLN(json_data.as<String>());

    // Convert JSON to string
    String jsonString;
    serializeJson(json_data, jsonString);

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
      // DEBUG_PRINTLN(response["digital_outputs"].as<String>());
      // Iterate through the data and control the pins accordingly
      for (JsonObject item : response["digital_outputs"].as<JsonArray>())
      {
        if (item["name"] == "Luz")
        {
          // DEBUG_PRINTLN((bool)item["value"]);
          digitalWrite(PIN_LUZ, !(bool)item["value"]);
        }
        else if (item["name"] == "Cocina")
        {
          // DEBUG_PRINTLN((bool)item["value"]);
          digitalWrite(PIN_COCINA, !(bool)item["value"]);
        }
      }
    }
    else
    {
      // An error occurred
      DEBUG_PRINT("An error occurred: ");
      DEBUG_PRINTLN(httpResponseCode);
    }

    http.end();
  }

  delay(3000);
}