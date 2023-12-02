#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define CAMERA_MODEL_WROVER_KIT

#include "camera_pins.h"

// Pin konfigurasi
const int moistureSensorPin = 32; // Pin untuk soil moisture sensor
const int LEDPin = 33;             // Pin untuk LED

// ===========================
// Masukkan informasi Wi-Fi
// ===========================
const char* ssid = "AYACHI";
const char* password = "12345678";
const char* mqtt_server = "192.168.137.207";
const char* MQTT_username = "jazmy"; 
const char* MQTT_password = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);

void startCameraServer();
void setupLedFlash(int pin);

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32CAMClient", MQTT_username, MQTT_password)) {
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
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
    // Inisialisasi pin
  pinMode(moistureSensorPin, INPUT);
  pinMode(LEDPin, OUTPUT);
  

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  client.setServer(mqtt_server, 1884);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();

  int moistureLevel = analogRead(moistureSensorPin);
  Serial.println(moistureLevel);

  const char* servoState = (moistureLevel > 2000) ? "on" : "off";
  int currentHour = timeClient.getHours() + 7;
  // digitalWrite(LEDPin, (currentHour >= 18 || currentHour < 6) ? HIGH : LOW); Default format 24 jam
  digitalWrite(LEDPin, (currentHour >= 18 || currentHour < 6) ? HIGH : LOW);
  int ledlevel = digitalRead(LEDPin);
  Serial.println(currentHour);

  StaticJsonDocument<300> doc;
  doc["timestamp"] = formattedTime;
  JsonArray values = doc.createNestedArray("values");
  
  JsonObject servoObject = values.createNestedObject();
  servoObject["servo"] = servoState;
  
  JsonObject humidityObject = values.createNestedObject();
  humidityObject["humidity"] = moistureLevel;

  char jsonBuffer[300];
  serializeJson(doc, jsonBuffer);

  client.publish("servo/control", jsonBuffer);

  delay(1000); // Delay 1 detik
}
