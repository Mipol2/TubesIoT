#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

const char* ssid = "AYACHI";
const char* password = "12345678";
const char* mqtt_server = "192.168.137.207";
const char* MQTT_username = "jazmy"; 
const char* MQTT_password = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo; 

void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi.");
}

void callback(char* topic, byte* message, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);

    String messageTemp;
    for (int i = 0; i < length; i++) {
        messageTemp += (char)message[i];
    }

    StaticJsonDocument<300> doc;
    deserializeJson(doc, messageTemp);

    int humidity = doc["values"][1]["humidity"];
    String servoState = doc["values"][0]["servo"].as<String>(); 

    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Servo state: ");
    Serial.println(servoState);

    if (humidity > 2000) {
        myservo.write(90); 
        Serial.println("Servo turned to 90 degrees due to high humidity.");
    } else {
        myservo.write(0); 
        Serial.println("Servo returned to 0 degrees.");
    }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32ServoClient", MQTT_username, MQTT_password)) {
      Serial.println("Connected to MQTT.");
      client.subscribe("servo/control");
    } else {
      Serial.print("Failed to connect. MQTT retry in 5 seconds. Error Code: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1884);
  client.setCallback(callback);

  myservo.attach(12);
  Serial.println("Setup complete.");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
