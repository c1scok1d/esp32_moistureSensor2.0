#include <WiFi.h>
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

#define MINUTES_BETWEEN_SLEEP 1
#define SENSORPIN 35

#define FORMAT_SPIFFS_IF_FAILED true

String apiKeyValue = "gwGWZKjADUeHe1f06muhnhdt38pmVwBaNuiyL18WvLHLMeFUZYcqOZqsgvyl";

WiFiClient client;

// Variables to save values from bluetooh setup
String hostname;
String sensorName;
String sensorLocation;
String ssid;
String pass;
esp_sleep_wakeup_cause_t wakeup_reason;

// File paths to save input values permanently
const char *SSID_path = "/ssid.txt";
const char *password_path = "/pass.txt";
const char *sensorName_path = "/name.txt";
const char *sensorLocation_path = "/location.txt";
const char *lastCapture_path = "/lastCapture.txt";

// Timer variables
unsigned long previousMillis = 0;
const long one_hour = 3600;
const long three_min = 180000;

float vpd = 0.00;
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 180

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

int currentVersionNumber = 1658075072;

esp32FOTA esp32FOTA("rodlandFarms", currentVersionNumber);

// bluetooth wireless configuration
BluetoothSerial SerialBT;

// battery meter
#define batteryPin 34 
int batterySensorValue;
float batteryCalibration = 0.36;
int batPercentage;

String readMoisture()
{
  const int AirValue = 4095;
  const int WaterValue = 1535;
  int reading = analogRead(SENSORPIN);
  Serial.print("\nMoisture Reading: ");
  Serial.println(reading);
  int moisture = map(reading, AirValue, WaterValue, 0, 100);

  return String(moisture);
}

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
/*
// initialize wi-fi
bool initWiFi()
{
  ssid = readFile(SPIFFS, SSID_path);
  pass = readFile(SPIFFS, password_path);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Pass: ");
  Serial.println(pass);

  if (ssid == "" || pass == "")
  {
    Serial.println("Undefined SSID.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname.c_str());

  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print("Connecting to WiFi");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    // wait 10 seconds before re-trying
    delay(3000);
    currentMillis = millis();
    if (currentMillis - previousMillis >= three_min)
    {
      Serial.println();
      Serial.println("Failed to connect");
      // delay(500);
      Serial.println("Going to sleep now");
      delay(500);
      Serial.flush();
      esp_deep_sleep_start();
    }
  }
  return true;
} */

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
String battery()
{
  batterySensorValue = analogRead(batteryPin);
  float voltage = (((batterySensorValue * 3.3) / 1024) * 2 + batteryCalibration);
  batPercentage = map(voltage, 2.8, 4.2, 0, 100);
  if (batPercentage >= 100)
  {
    batPercentage = 100;
  }
  if (batPercentage <= 0)
  {
    batPercentage = 1;
  }

  Serial.print("Battery Percentage: ");
  Serial.println(batPercentage);
  return String(batPercentage);
}
// upload sensor readings to api
void uploadReadings()
{
  const char *serverName = "http://athome.rodlandfarms.com";
  String path = "/api/esp/data";

  HTTPClient http;
  http.begin(serverName + path);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Prepare HTTP POST request data (post data will be determined by sensor that are detected
  String httpRequestData = "api_token=" + apiKeyValue +
                           "&hostname=" + hostname +
                           "&sensor=" + sensorName +
                           "&location=" + sensorLocation +
                           "&moisture=" + String(readMoisture()) +
                           "&batt=" + battery();

  // Send HTTP POST request
  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode == 200)
  {
    Serial.println(httpRequestData);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
  }
  else
  {
    Serial.println(httpRequestData);
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources and go to sleep
  http.end();
  delay(500);
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
    // Serial.println(serverName + path);
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
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
          // Serial.print("\n");
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
  // Free resources and go to sleep
  http.end();
}
/*
void initBlueTooth()
{
  Serial.print("Hostname:\t");
  Serial.println(hostname);
  SerialBT.register_callback(BTcallback);
  SerialBT.begin("RodlandFarms-" + hostname);
  Serial.println(F("The device started, now you can pair it with bluetooth!"));
  Serial.println("Enter wifi Name");
ab:
  if (Serial.available())
  {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available())
  {
    ssid = SerialBT.readString();
    // Write value to file
    writeFile(SPIFFS, SSID_path, ssid.c_str());
  }
  else
  {
    goto ab;
  }

  Serial.println("Enter password");
ac:
  if (Serial.available())
  {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available())
  {
    pass = SerialBT.readString().c_str();
    // Write value to file
    writeFile(SPIFFS, password_path, pass.c_str());
  }
  else
  {
    goto ac;
  }

  Serial.println("Enter plant name");
ad:
  if (Serial.available())
  {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available())
  {
    sensorName = SerialBT.readString().c_str();
    // Write value to file
    writeFile(SPIFFS, sensorName_path, sensorName.c_str());
  }
  else
  {
    goto ad;
  }

  Serial.println("Enter plant location");
ae:
  if (Serial.available())
  {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available())
  {
    sensorLocation = SerialBT.readString().c_str();
    // Write value to file
    writeFile(SPIFFS, sensorLocation_path, sensorLocation.c_str());
  }
  else
  {
    goto ae;
  }
  Serial.print("Saved SSID: ");
  Serial.println(ssid);
  Serial.print("Saved pass: ");
  Serial.println(pass);
  if (ssid != "" && pass != "")
  {
    ESP.restart();
  }
}
*/
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(500);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("-------------------------------------");
  Serial.println("       Rodland Farms @HOME           ");
  Serial.print("        Version: ");
  Serial.println(currentVersionNumber);
  Serial.println("-------------------------------------");
  Serial.println();

  esp32FOTA.checkURL = "http://athome.rodlandfarms.com/firmware.json";
  SPIFFS.begin(true);

  if (!SPIFFS.begin((FORMAT_SPIFFS_IF_FAILED)))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // if (!initWiFi())
  // {
  //   initBlueTooth();
  //   unsigned long currentMillis = millis();
  //   previousMillis = currentMillis;
  //   while (WiFi.status() != WL_CONNECTED) {
  //     Serial.print(".");
  //     delay(3000);
  //     currentMillis = millis();
  //     if (currentMillis - previousMillis >= three_min) {
  //       Serial.println("Failed to connect.");
  //       //delay(500);
  //       Serial.println("Going to sleep now");
  //       delay(500);
  //       Serial.flush();
  //       esp_deep_sleep_start();
  //     }
  //   }
  // }
  // your code above is commented out so that you can remove it as required.
  prov_main(); // function that is responsible for provisioning through BLE and Wifi connection
  // Program will wait here until it has been provisioned and connected with wifi


  // else
  // {
  //checkUpdate();
  delay(3000);
  hostname = WiFi.macAddress();
  hostname.replace(":", ""); // remove : from mac address
  sensorName = readFile(SPIFFS, sensorName_path).c_str();
  sensorLocation = readFile(SPIFFS, sensorLocation_path).c_str();
  uploadReadings();
  // }
  Serial.println("Going to sleep now");
  delay(500);
  Serial.flush();
  esp_deep_sleep_start();
}

void loop()
{
}
