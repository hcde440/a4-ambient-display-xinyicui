#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include "Wire.h"
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include "pitches.h"

// Wifi
//#define wifi_ssid "University of Washington"
//#define wifi_password ""

#define wifi_ssid "Daddy"
#define wifi_password "980210980805"

// MQTT
#define mqtt_server "mediatedspaces.net"
#define mqtt_user "hcdeiot"
#define mqtt_password "esp8266"

WiFiClient espClient;
PubSubClient mqtt(espClient);
DynamicJsonBuffer jsonBuffer;

char mac[6];       //unique id
char message[201]; //setting message length

const int red = 12;    // red LED connected to pin 12
const int yellow = 13; // yellow LED connected to pin 13
const int green = 15;  // green LED connected to pin 15
int buzzer = 14; // buzzer connected to pin 14

//Piano Notes
int c = N_C5; //Plays C Note
int d = N_D5; //Plays D Note
int e = N_E5; //Plays E Note
int f = N_F5; //Plays F Note
int g = N_G5; //Plays G Note
int a = N_A5; //Plays A Note
int b = N_B5; //Plays B Note
int duration(250); // wait for 250 miliseconds

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

  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  digitalWrite(buzzer, LOW);
}

void setup_wifi() {
  delay(500);
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

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();
  delay(1000);
}

void callback(char* topic, byte* line, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  JsonObject& root = jsonBuffer.parse(line);

  if (!root.success()) {
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }
  int AQI = root["Air Quality Index"];
  double tempC = root["Temperature in Celsius"];
  Serial.println(AQI);
  Serial.println(tempC);

  if (AQI >= 150) {
    digitalWrite(red, HIGH);
    digitalWrite(yellow, LOW);
    digitalWrite(green, LOW);
  } else if (AQI >= 80 && AQI < 150) {
    digitalWrite(yellow, HIGH);
    digitalWrite(red, LOW);
    digitalWrite(green, LOW);
  } else if (AQI < 80) {
    digitalWrite(green, HIGH);
    digitalWrite(yellow, LOW);
    digitalWrite(red, LOW);
  }

  if (tempC >= 30) {
    tone(buzzer, c, duration);
  } else if (tempC >= 20 && tempC < 30) {
    tone(buzzer, d, duration);
  } else if (tempC >= 10 && tempC < 20) {
    tone(buzzer, e, duration);
  } else if (tempC >= 0 && tempC < 10) {
  tone(buzzer, f, duration);
  }else if (tempC < 0){
    tone(buzzer, g, duration);
  }
}
