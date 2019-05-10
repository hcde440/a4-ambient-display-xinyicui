//Libraries to include
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "Wire.h"
#include <PubSubClient.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <Adafruit_MPL115A2.h>
#include <SPI.h>

// Wifi
//#define wifi_ssid "University of Washington"
//#define wifi_password ""

#define wifi_ssid "Daddy"
#define wifi_password "980210980805"

// MQTT
#define mqtt_server "mediatedspaces.net"
#define mqtt_user "hcdeiot"
#define mqtt_password "esp8266"

Adafruit_MPL115A2 mpl115a2;

WiFiClient espClient;
PubSubClient mqtt(espClient);

//BreezoMeter API
const char* key = "ed6906f6d53b425fb3e1a0b2498d57ea";

// Seattle's latitude and longitude: 47.6062° N, 122.3321° W
const char* lat = "47.6062";
const char* lon = "-122.3321";

const char* host = "api.breezometer.com";
const int httpsPort = 443;
unsigned long timer;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "0C A9 23 4F 27 B3 17 DA AD E7 4C 0B EA 61 C5 84 F2 8D C3 DA";

char mac[6];       //unique id
char message[201]; //setting message length

// Setup Wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");         //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());         //.macAddress returns a byte array 6 bytes representing the MAC address
  WiFi.macAddress().toCharArray(mac, 6);     //5C:CF:7F:F0:B0:C1 for example
}

// function to reconnect if we become disconnected from the server
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("theSunnyTopic/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

void setup() {

  Serial.begin(115200);

  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));

  while (! Serial);

  setup_wifi();                      //start wifi

  mqtt.setServer(mqtt_server, 1883); //start the mqtt server
  mqtt.setCallback(callback);        //register the callback function

  timer = millis();

  mpl115a2.begin(); // initialize barometric sensor
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  getMet();

  // Read data from sensor: MPL115A2 values every minute
  if (millis() - timer > 60000) {
    float tempC = mpl115a2.getTemperature();  // start the sensor
    Serial.print("Reading from MPL: "); Serial.print(tempC, 1); Serial.println(" °C");
    char str_tempC[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character

    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_tempC
    dtostrf(tempC, 5, 2, str_tempC);
    sprintf(message, "{\"Temperature in Celsius\":\"%s\"}", str_tempC);
    mqtt.publish("theSunnyTopic/Temp", message);
    Serial.println("publishing");
  }
  delay(60000);
}

void getMet() {
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  String url = (String("https://api.breezometer.com/air-quality/v2/current-conditions?lat=") + lat + "&lon=" + lon + "&key=" + key);
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parse(line);

  String aqi = root["data"]["indexes"]["baqi"]["aqi"].as<String>();
  Serial.println(aqi);

  sprintf(message, "{\"Air Quality Index\":\"%s\"}", aqi.c_str());
  mqtt.publish("theSunnyTopic/AQI", message);
}

void callback(char* topic, byte* line, unsigned int timer) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(line);

  if (!root.success()) {
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }
}
