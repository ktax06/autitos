#include <WiFi.h>
#include <WebSocketsClient.h> // Usamos la librería para WebSocket
#include <ArduinoJson.h> // Librería para manejo de JSON
#include "esp_camera.h" // Librería para la cámara ESP32
#include "base64.h" // Librería para codificar imágenes a Base64
#include <driver/ledc.h> // Librería para PWM
#include <ESPmDNS.h>

#define LEDC_RESOLUTION 8  // 8 bits = valores de 0-255

//WiFi
const char* ssid = "POCO X3 NFC";
const char* password = "11111111";

//WebSocket
WebSocketsClient webSocket;
const char* host = "192.168.47.159"; // IP del servidor WebSocket
const uint16_t port = 3000;
const char* path = "/ws";

//Configuración de cámara
void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error iniciando cámara: 0x%x", err);
    return;
  }
}

//Enviar imagen al servidor
void sendImage() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Error capturando imagen");
    return;
  }

  String imageBase64 = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  // JSON con tipo imagen
  DynamicJsonDocument doc(200000); // cuidado con el tamaño
  doc["type"] = "image";
  doc["value"] = imageBase64;

  String output;
  serializeJson(doc, output);

  webSocket.sendTXT(output);
}

//Recibir comando
void handleCommand(const String& cmd) {
  Serial.print("Comando recibido: ");
  Serial.println(cmd);

  if (cmd == "forward") {
    moveForward();
  } else if (cmd == "backward") {
    moveBackward();
  } else if (cmd == "left") {
    turnLeft();
  } else if (cmd == "right") {
    turnRight();
  } else if (cmd == "stop") {
    stopMotors();
  }
}

//Evento WebSocket
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Conectado al WebSocket");
      break;
    case WStype_DISCONNECTED:
      Serial.println("Desconectado del WebSocket");
      break;
    case WStype_TEXT:
      Serial.println("Mensaje recibido");
      DynamicJsonDocument doc(512);
      DeserializationError err = deserializeJson(doc, payload);
      if (!err && doc["type"] == "command") {
        handleCommand(doc["value"].as<String>());
      }
      break;
  }
}

// Configuración de motores
#define ENA 12
#define IN1 14
#define IN2 27

#define ENB 13
#define IN3 26
#define IN4 25

#define CHANNEL_A 0
#define CHANNEL_B 1

void setupMotors() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Ahora, adjuntamos los pines para PWM (ajustando frecuencia y resolución)
  ledcAttach(ENA, CHANNEL_A, LEDC_RESOLUTION );  // Asocia el pin ENA al canal A
  ledcWrite(CHANNEL_A, 5000);  // Frecuencia 5kHz

  ledcAttach(ENB, CHANNEL_B, LEDC_RESOLUTION);  // Asocia el pin ENB al canal B
  ledcWrite(CHANNEL_B, 5000);  // Frecuencia 5kHz
}

void setSpeed(uint8_t speed) {
  ledcWrite(CHANNEL_A, speed); // 0-255
  ledcWrite(CHANNEL_B, speed);
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  setSpeed(200); // Ajusta la velocidad aquí
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  setSpeed(200);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  setSpeed(200);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  setSpeed(200);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  setSpeed(0);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando conexión Wi-Fi...");

  // Conexión WiFi
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("WiFi conectado");
  // Serial.println(WiFi.localIP());

  // // Wifi con access point
  // Serial.println("Configurando Punto de Acceso...");

  // WiFi.softAP(ssidAP, passwordAP);

  // IPAddress IP = WiFi.softAPIP();
  // Serial.print("Dirección IP del Punto de Acceso: ");
  // Serial.println(IP);

  // // Opcional: Configurar un nombre de host para el ESP32
  // if (!MDNS.begin("esp32-ap")) {
  //   Serial.println("Error al configurar mDNS!");
  // }
  // Serial.println("Nombre de host mDNS: esp32-ap.local");

  startCamera();

  // Configuración WebSocket
  webSocket.begin(host, port, path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  setupMotors();  // Inicializar motores
}

void loop() {
  webSocket.loop();

  static unsigned long lastImageTime = 0;
  if (millis() - lastImageTime > 300) { // cada 300ms
    sendImage();
    lastImageTime = millis();
  }
}