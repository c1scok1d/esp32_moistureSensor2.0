/*
// ===============================
// AUTHOR:        Rodland Farms
// CREATE DATE:   August 7, 2022
// PURPOSE:       Home plant moisture sensor
// SPECIAL NOTES: Version 2.0
// ===============================
// Change History:
//
//==================================
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>
#include <esp32fota.h>
#include <Arduino.h>
#include "SPI.h"
#include <Wire.h>
#include <DHT.h>
#include <HTTPClient.h>
#include "BluetoothSerial.h"
#include "HttpsOTAUpdate.h"
#include "Update.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "ArduinoJson.h"
#include "provision.h"
#include "rest_methods.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"

// format if not spiffs
#define FORMAT_SPIFFS_IF_FAILED true

String apiKey = "gwGWZKjADUeHe1f06muhnhdt38pmVwBaNuiyL18WvLHLMeFUZYcqOZqsgvyl";

WiFiClient client;

// Variables to save values from bluetooh setup
// String hostname;
// String sensorName;
// String sensorLocation;
// String ssid;
// String pass;

// ota software update
static const char *server_certificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw\n"
    "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
    "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw\n"
    "WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n"
    "RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
    "AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP\n"
    "R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx\n"
    "sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm\n"
    "NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg\n"
    "Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG\n"
    "/kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB\n"
    "Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA\n"
    "FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw\n"
    "AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw\n"
    "Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB\n"
    "gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W\n"
    "PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl\n"
    "ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz\n"
    "CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm\n"
    "lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4\n"
    "avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2\n"
    "yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O\n"
    "yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids\n"
    "hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+\n"
    "HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv\n"
    "MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX\n"
    "nLRbwHOoq7hHwg==\n"
    "-----END CERTIFICATE-----\n";

int currentVersionNumber = 1659916189;
esp32FOTA esp32FOTA("rodlandFarms", currentVersionNumber);

// bluetooth wireless configuration
BluetoothSerial SerialBT;

// Read wifi configuration values from file
String readFile(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// write bluetooth WIFI configuration values to file
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("− failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("− file written");
  }
  else
  {
    Serial.println("− frite failed");
  }
}

// get and calculate moisture value
int readMoisture()
{
#define SENSORPIN 35
  int AirValue = 0;
  int WaterValue = 4095;
  int count = 0;
  int reading = analogRead(SENSORPIN);

  if (reading ==0){
    while(reading ==0 && count < 10){
      delay(1000);
      reading = analogRead(SENSORPIN);
      Serial.print(count);
    }
  }

  int moisture = map(reading, AirValue, WaterValue, 0, 100);

  Serial.println((String) "\nMoisture Reading:\t" + reading + "\nMoisture Percent:\t" + moisture + "%");
  return moisture;
}

// get current power level
float getBattery()
{
  const int MAX_ANALOG_VAL = 4095;
  const float MAX_BATTERY_VOLTAGE = 4.2; // Max LiPoly voltage of a 3.7 battery is 4.2

  int rawValue = analogRead(34);

  // Reference voltage on ESP32 is 1.1V
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html#adc-calibration
  // See also: https://bit.ly/2zFzfMT
  float voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3; // calculate voltage level
  float batteryFraction = voltageLevel / MAX_BATTERY_VOLTAGE;

  Serial.println((String) "\nBattery Reading:\t" + rawValue + "\nVoltage Reading:\t" + voltageLevel + "\nVoltage Percent:\t" + (batteryFraction * 100) + "%");

  if ((batteryFraction * 100) > 100)
  {
    return (100);
  }
  else
  {
    return (batteryFraction * 100);
  }
}

// upload sensor readings to api
String uploadReadings(int moisture, float battery, String hostname, String sensorName, String sensorLocation)
{
  const char *serverName = "http://athome.rodlandfarms.com";
  String server_path = "/api/esp/data";
  String server_uri = serverName + server_path;

  // Prepare HTTP POST request data (post data will be determined by sensor that are detected
  String httpRequestData = "api_token=" + apiKey +
                           "&hostname=" + hostname +
                           "&sensor=" + sensorName +
                           "&location=" + sensorLocation +
                           "&moisture=" + String(moisture) +
                           "&batt=" + String(battery);
  // Send HTTP POST request
  int httpResponseCode = POST(server_uri, httpRequestData);

  return String(httpRequestData);
}

// http events for over the air firmware update
void HttpEvent(HttpEvent_t *event)
{
  switch (event->event_id)
  {
  case HTTP_EVENT_ERROR:
    Serial.println("Http Event Error");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    Serial.println("Http Event On Connected");
    break;
  case HTTP_EVENT_HEADER_SENT:
    Serial.println("Http Event Header Sent");
    break;
  case HTTP_EVENT_ON_HEADER:
    Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    break;
  case HTTP_EVENT_ON_FINISH:
    Serial.println("Http Event On Finish");
    break;
  case HTTP_EVENT_DISCONNECTED:
    Serial.println("Http Event Disconnected");
    break;
  }
}

// over the air firmware update
void checkUpdate()
{
  static const char *url = "https://athome.rodlandfarms.com/firmware.bin";
  static HttpsOTAStatus_t otastatus = HttpsOTA.status();
  int i = 0;

  const char *serverName = "https://athome.rodlandfarms.com";
  String path = "/firmware.json";
  HTTPClient http;
  http.useHTTP10(true);
  http.begin(serverName + path);

  // Send HTTP POST request
  int httpResponseCode = http.GET();
  // Parse response
  DynamicJsonDocument doc(200);
  deserializeJson(doc, http.getStream());

  if (httpResponseCode == 200)
  {
    if (doc["version"].as<String>() != String(currentVersionNumber))
    {
      String firmwareVersion = doc["version"];
      Serial.println("\nVersion Mismatch");
      Serial.print("Server Version: ");
      Serial.println(firmwareVersion);
      Serial.print("Running Version: ");
      Serial.println(currentVersionNumber);
      Serial.println();
      HttpsOTA.onHttpEvent(HttpEvent);
      Serial.println("\nStarting OTA Update");
      HttpsOTA.begin(url, server_certificate);

      Serial.print("Please Wait it takes some time");

      while (otastatus != HTTPS_OTA_SUCCESS)
      {
        otastatus = HttpsOTA.status();
        if (i == 3000)
        {
          Serial.print(".");
          i = 0;
        }
        i++;
        if (otastatus == HTTPS_OTA_SUCCESS)
        {
          Serial.println("Firmware written successfully. Rebooting in 3 seconds...");
          delay(3000);
          ESP.restart();
        }
        else if (otastatus == HTTPS_OTA_FAIL)
        {
          Serial.println("Firmware Upgrade Fail");
        }
      }
    }
  }
  else
  {
    Serial.println(serverName + path);
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(500);

  Serial.println("-------------------------------------");
  Serial.println("       Rodland Farms @HOME           ");
  Serial.print("       Version: ");
  Serial.println(currentVersionNumber);
  Serial.println("-------------------------------------");
  Serial.println();
  // File paths to save input values permanently
  const char *SSID_path = "/ssid.txt";
  const char *password_path = "/pass.txt";
  const char *sensorName_path = "/name.txt";
  const char *sensorLocation_path = "/location.txt";

  esp_sleep_wakeup_cause_t wakeup_reason;
  #define uS_TO_S_FACTOR 1000000UL /* Conversion factor for micro seconds to seconds */
  #define TIME_TO_SLEEP 60         /* Time ESP32 will go to sleep (in seconds) 1min */
  // #define TIME_TO_SLEEP 3600 /* Time ESP32 will go to sleep (in seconds) 1hr */
  // #define TIME_TO_SLEEP 144000  /* Time ESP32 will go to sleep (in seconds) 4hrs */
  //  set sleep timer
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // set up OTA firmware update URL
  esp32FOTA.checkURL = "http://athome.rodlandfarms.com/firmware.json";

  // check and load SPIFFS file system
  SPIFFS.begin(true);
  if (!SPIFFS.begin((FORMAT_SPIFFS_IF_FAILED)))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  prov_main(); // if provisioned start wifi, otherwise provision wifi via ble
  int moisture = readMoisture();
  if (moisture != 0){
    // get moisture and battery level readings
    float battery = getBattery();

    String hostname = WiFi.macAddress();
    hostname.replace(":", ""); // remove : from mac address
    WiFi.setHostname("Rodland Farms");

    String sensorName = readFile(SPIFFS, sensorName_path).c_str();
    String sensorLocation = readFile(SPIFFS, sensorLocation_path).c_str();

    uploadReadings(moisture, battery, hostname, sensorName, sensorLocation);
    //checkUpdate(); // check if firmware update is available
  }
  Serial.println("\nGoing to sleep now");
  esp_deep_sleep_start();
}

void loop()
{
}
