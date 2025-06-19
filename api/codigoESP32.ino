#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_camera.h"
#include <ArduinoJson.h>

WebSocketsClient webSocket;

const size_t CAPACITY = JSON_OBJECT_SIZE(3) + 30;


const char* ssid = "autoicc";
const char* password = "autitos1";

unsigned long lastImageSent = 0;
const unsigned long imageInterval = 200; // ms

#define ENAS 2
#define IN1 12
#define IN2 13
#define IN3 15
#define IN4 14 
#define FLASH_PIN 4

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_TEXT: {
      Serial.printf("Texto recibido por WebSocket: %s\n", (char*)payload);

      StaticJsonDocument<CAPACITY> doc;

      DeserializationError error = deserializeJson(doc, (char*)payload);

      if (error) {
        return;
      }

      const char* msgType = doc["type"];

      if (msgType && String(msgType) == "action") {
        const char* actionValue = doc["value"];
        int velocidad = map(doc["speed"] | 255, 0, 100, 100, 255);
        if (actionValue) {
          executeAction(String(actionValue), velocidad);
        }
      }
      break;
    }
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      webSocket.sendTXT("{\"role\":\"esp32\"}");
      break;
    case WStype_BIN:
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  WiFi.setSleep(false);

  if (!initCamera()) {
  }

  configureMotorPins();

  webSocket.begin("api-autito.tu-dominio.com", 80, "/ws"); // Cambia a tu URL de WebSocket
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  handleFlashOn();
  delay(500);
  handleFlashOff();
  delay(500);
  handleFlashOn();
  delay(500);
  handleFlashOff();
}

void loop() {
  webSocket.loop();

  unsigned long now = millis();
  if (now - lastImageSent > imageInterval && webSocket.isConnected()) {
    sendCameraImage();
    lastImageSent = now;
  }
}

void sendCameraImage() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    return;
  }

  webSocket.sendTXT("{\"type\":\"image\"}");
  delay(10);
  webSocket.sendBIN(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void executeAction(String action, int velocidad) {
  if (action == "forward") handleForward(velocidad);
  else if (action == "backward") handleBackward(velocidad);
  else if (action == "left") handleLeft(velocidad);
  else if (action == "right") handleRight(velocidad);
  else if (action == "stop") handleStop();
  else if (action == "flash_on") handleFlashOn();
  else if (action == "flash_off") handleFlashOff();
}

void configureMotorPins() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENAS, OUTPUT);
  analogWrite(ENAS, 0);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);
}

void handleForward(int velocidad) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENAS, velocidad);
}

void handleBackward(int velocidad) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENAS, velocidad);
}

void handleLeft(int velocidad) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENAS, 120);
}

void handleRight(int velocidad) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENAS, 120);
}

void handleStop() {
  analogWrite(ENAS, 0);
}

void handleFlashOn() {
  digitalWrite(FLASH_PIN, HIGH);
}

void handleFlashOff() {
  digitalWrite(FLASH_PIN, LOW);
}

bool initCamera() {
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}