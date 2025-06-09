#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_camera.h"

WebSocketsClient webSocket;

const char* ssid = "autoicc";
const char* password = "autitos1";

#define ENAS 2
#define IN1 12
#define IN2 13
#define IN3 15
#define IN4 14 
#define FLASH_PIN 4

// Pines cámara
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
      String action = String((char*)payload);
      Serial.printf("Acción recibida por WebSocket: %s\n", action.c_str());
      executeAction(action);
      break;
    }
    case WStype_DISCONNECTED:
      Serial.println("WebSocket desconectado");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket conectado");
      webSocket.sendTXT("ESP32 listo");
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
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado, IP: " + WiFi.localIP().toString());

  if (!initCamera()) {
    Serial.println("Error al inicializar cámara");
  }

  configureMotorPins();

  // Conexión WebSocket
  webSocket.begin("api-autito.arturoalvarez.website", 80, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);  // reconexión cada 5s
}

void loop() {
  webSocket.loop();

  // (opcional) enviar mensaje cada 5s
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) {
    lastSend = millis();
    webSocket.sendTXT("Hola desde ESP32");
  }
}

void executeAction(String action) {
  if (action == "forward") handleForward();
  else if (action == "backward") handleBackward();
  else if (action == "left") handleLeft();
  else if (action == "right") handleRight();
  else if (action == "stop") handleStop();
  else if (action == "flash_on") handleFlashOn();
  else if (action == "flash_off") handleFlashOff();
  else Serial.println("Acción desconocida: " + action);
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

void handleForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENAS, 255);
}

void handleBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENAS, 255);
}

void handleLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENAS, 255);
}

void handleRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENAS, 255);
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

// Inicializa la cámara
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