#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"

const char* ssid = "Casa1";
const char* password = "argentina";

const char* serverUrl = "http://192.168.1.33:8000/esp32/action";

#define ENA 4
#define IN1 12
#define IN2 13

#define ENB 2
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

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32-CAM...");

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
  Serial.println("Setup completo");
}

void loop() {
  String action = getActionFromServer();
  if (action.length() > 0) {
    executeAction(action);
  } else {
    Serial.println("No action or error getting action");
  }

  bool ok = sendCameraImage("http://192.168.1.33:8000/esp32/upload_image");
  
  if (ok) {
    Serial.println("Imagen enviada correctamente");
  } else {
    Serial.println("Error enviando imagen");
  }

  delay(1000);  // Esperar 3 segundos antes de la siguiente consulta
}

String getActionFromServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi no conectado");
    return "";
  }

  HTTPClient http;
  http.begin(serverUrl);

  int httpCode = http.GET();
  String payload = "";

  if (httpCode == 200) {
    payload = http.getString();
    Serial.println("Respuesta servidor: " + payload);
  } else {
    Serial.printf("Error en GET: %d\n", httpCode);
  }

  http.end();

  // Ejemplo respuesta: {"action": "forward"}
  // Extraer el valor de "action" de forma simple

  int start = payload.indexOf("\"action\"");
  if (start == -1) return "";

  int colon = payload.indexOf(":", start);
  int quote1 = payload.indexOf("\"", colon);
  int quote2 = payload.indexOf("\"", quote1 + 1);
  if (colon == -1 || quote1 == -1 || quote2 == -1) return "";

  String action = payload.substring(quote1 + 1, quote2);
  return action;
}

void executeAction(String action) {
  Serial.println("Ejecutando acción: " + action);

  if (action == "forward") {
    handleForward();
  } else if (action == "backward") {
    handleBackward();
  } else if (action == "left") {
    handleLeft();
  } else if (action == "right") {
    handleRight();
  } else if (action == "stop") {
    handleStop();
  } else if (action == "flash_on") {
    handleFlashOn();
  } else if (action == "flash_off") {
    handleFlashOff();
  } else {
    Serial.println("Accion desconocida");
  }
}

// Implementación motores y flash

void configureMotorPins() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);

  Serial.println("Pines de motores configurados");
}

void handleForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
  Serial.println("Avanzando");
}

void handleBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
  Serial.println("Retrocediendo");
}

void handleLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
  Serial.println("Girando izquierda");
}

void handleRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
  Serial.println("Girando derecha");
}

void handleStop() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  Serial.println("Parando motores");
}

void handleFlashOn() {
  digitalWrite(FLASH_PIN, HIGH);
  Serial.println("Flash encendido");
}

void handleFlashOff() {
  digitalWrite(FLASH_PIN, LOW);
  Serial.println("Flash apagado");
}

// Cámara

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
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error al inicializar cámara: 0x%x\n", err);
    return false;
  }
  return true;
}

bool sendCameraImage(const char* serverUrl) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Error obteniendo frame de cámara");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi no conectado");
    esp_camera_fb_return(fb);
    return false;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");

  int httpResponseCode = http.POST(fb->buf, fb->len);

  esp_camera_fb_return(fb);

  if (httpResponseCode > 0) {
    Serial.printf("POST enviado, código respuesta: %d\n", httpResponseCode);
    return true;
  } else {
    Serial.printf("Error en POST: %s\n", http.errorToString(httpResponseCode).c_str());
    return false;
  }

  http.end();
}