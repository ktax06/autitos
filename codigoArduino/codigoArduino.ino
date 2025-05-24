#include <WiFi.h>
#include <ArduinoJson.h> // Librería para manejo de JSON
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_camera.h"
#include "esp_http_server.h"

//WiFi
const char* ssid = "Casa1";
const char* password = "argentina";

//WebSocket Server
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); // Puerto para WebSocket

// Configuración de motores
#define ENA 4  // Enable Motor A (PWM)
#define IN1 12  // Motor A input 1
#define IN2 13  // Motor A input 2

#define ENB 2  // Enable Motor B (PWM)
#define IN3 15  // Motor B input 1
#define IN4 14  // Motor B input 2

// Configuración del flash/LED
#define FLASH_PIN 4  // Ajusta este pin según tu configuración

// Variables para movimiento
int currentSpeed = 0;
bool isMoving = false;
String currentDirection = "STOP";

// Configuración de la cámara para ESP32-CAM AI Thinker
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

// Servidor de streaming
httpd_handle_t stream_httpd = NULL;

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
  
  // Calidad y resolución iniciales
  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Inicializar la cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("La inicialización de la cámara falló con error 0x%x", err);
    return false;
  }
  
  return true;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[64];

  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=123456789000000000000987654321";
  static const char* _STREAM_BOUNDARY = "\r\n--123456789000000000000987654321\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Error al capturar la imagen de la cámara");
      res = ESP_FAIL;
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }
    
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)part_buf, strlen((const char *)part_buf));
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
    }
    
    if (res != ESP_OK) {
      break;
    }
  }

  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 82;  // Puerto para el streaming

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("Servidor de streaming iniciado en el puerto 82");
  }
}

void configureMotorPins() {
  // Configurar pines de control de motores como salidas
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  // Inicialmente en estado LOW
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  
  // Configurar flash
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);


  Serial.println("Motor pins configured");
}

// Función para mover hacia adelante
void moveForward(int speed) {
  Serial.println("Adelante con velocidad: " + String(speed));
  // Calcular el valor PWM basado en el porcentaje de velocidad (0-100 -> 0-255)
  int pwmValue = map(speed, 0, 100, 100, 255);
  
  // Motor A - Forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  // Motor B - Forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  // Aplicar velocidad
  analogWrite(ENA, pwmValue);
  analogWrite(ENB, pwmValue);
  
  currentDirection = "FORWARD";
  currentSpeed = speed;
  isMoving = true;
}

// Función para mover hacia atrás
void moveBackward(int speed) {
  Serial.println("Atrás con velocidad: " + String(speed));
  // Calcular el valor PWM basado en el porcentaje de velocidad (0-100 -> 0-255)
  int pwmValue = map(speed, 0, 100, 100, 255);
  
  // Motor A - Backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  // Motor B - Backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  // Aplicar velocidad
  analogWrite(ENA, pwmValue);
  analogWrite(ENB, pwmValue);
  
  currentDirection = "BACKWARD";
  currentSpeed = speed;
  isMoving = true;
}

// Función para girar a la izquierda
void turnLeft(int turn) {
  Serial.println("Izquierda con intensidad: " + String(turn));
  // Calcular el valor PWM basado en el porcentaje de giro (0-100 -> 0-255)
  int pwmValue = map(turn, 0, 100, 100, 255);
  
  // Motor A - Forward (más rápido)
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  // Motor B - Backward o más lento
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  // Aplicar velocidades diferentes a cada motor para girar
  analogWrite(ENA, pwmValue);
  analogWrite(ENB, pwmValue);
  
  currentDirection = "LEFT";
  currentSpeed = turn;
  isMoving = true;
}

// Función para girar a la derecha
void turnRight(int turn) {
  Serial.println("Derecha con intensidad: " + String(turn));
  // Calcular el valor PWM basado en el porcentaje de giro (0-100 -> 0-255)
  int pwmValue = map(turn, 0, 100, 100, 255);
  
  // Motor A - Backward o más lento
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  // Motor B - Forward (más rápido)
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  // Aplicar velocidades diferentes a cada motor para girar
  analogWrite(ENA, pwmValue);
  analogWrite(ENB, pwmValue);
  
  currentDirection = "RIGHT";
  currentSpeed = turn;
  isMoving = true;
}

// Función para detener el movimiento
void stopMovement() {
  // Detener ambos motores
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  // Asegurar que los PWM estén en 0
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  
  currentDirection = "STOP";
  currentSpeed = 0;
  isMoving = false;
  
  Serial.println("Detenido");
}

// Función para controlar el flash
void turnOnFlash() {
  digitalWrite(FLASH_PIN, HIGH);
  Serial.println("Flash encendido");
}

void turnOffFlash() {
  digitalWrite(FLASH_PIN, LOW);
  Serial.println("Flash apagado");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Desconectado!\n", num);
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      String url = (char *)payload;
      Serial.printf("[%u] Conectado desde %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], url.c_str());

      
      // Enviar mensaje de bienvenida y estado actual
      String welcomeMsg = "{\"status\":\"connected\", \"direction\":\"" + currentDirection + "\", \"speed\":" + String(currentSpeed) + "}";
      webSocket.sendTXT(num, welcomeMsg);
    }
    break;
    
    case WStype_TEXT: {
      Serial.printf("[%u] Texto recibido: %s\n", num, payload);
      
      // Procesar comandos JSON
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload, length);
      
      if (error) {
        Serial.print("deserializeJson() falló: ");
        Serial.println(error.c_str());
        return;
      }
      
      // Verificar si es un comando
      if (doc.containsKey("value")) {
        String command = doc["value"].as<String>();
        int speed = doc.containsKey("speed") ? doc["speed"].as<int>() : 50;  // Default 50% si no se especifica
        int turn = doc.containsKey("turn") ? doc["turn"].as<int>() : 70;     // Default 70% si no se especifica
        
        // Procesar el comando
        if (command == "FORWARD") {
          moveForward(speed);
        } 
        else if (command == "BACKWARD") {
          moveBackward(speed);
        } 
        else if (command == "LEFT") {
          turnLeft(turn);
        } 
        else if (command == "RIGHT") {
          turnRight(turn);
        } 
        else if (command == "STOP") {
          stopMovement();
        } 
        else if (command == "FLASH_ON") {
          turnOnFlash();
        } 
        else if (command == "FLASH_OFF") {
          turnOffFlash();
        }
        
        // Enviar respuesta con el estado actual
        String statusMsg = "{\"status\":\"ok\", \"direction\":\"" + currentDirection + "\", \"speed\":" + String(currentSpeed) + "}";
        webSocket.broadcastTXT(statusMsg);
      }
    }
    break;
    
    case WStype_BIN:
      Serial.printf("[%u] Recibidos %u bytes binarios\n", num, length);
      break;
      
    case WStype_PONG:
      Serial.printf("[%u] Recibido PONG\n", num);
      break;
  }
}

void handleRoot() {
  String html = "<html><head><title>ESP32 Robot Control</title></head>";
  html += "<body><h1>ESP32 Robot Control Server</h1>";
  html += "<p>Use WebSocket en puerto 81 para controlar el robot</p>";
  html += "<p>Dirección IP: " + WiFi.localIP().toString() + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Tiempo para que se establezca la conexión serial

  Serial.println("Iniciando configuración...");
  
  // Configurar pines de motor y LED
  configureMotorPins();

  // Inicializar cámara
  Serial.println("Inicializando cámara...");
  if (!initCamera()) {
    Serial.println("Fallo al inicializar la cámara. Reiniciando...");
    ESP.restart();
  }
  Serial.println("Cámara inicializada correctamente");

  Serial.println("Iniciando conexión Wi-Fi...");

  // Conexión WiFi
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFallo al conectar a WiFi. Reiniciando...");
    ESP.restart();
  }
  
  Serial.println("");
  Serial.println("Conectado a WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Iniciar el servidor HTTP
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");

  server.on("/scan", []() {
    server.enableCORS(true);
    server.send(200, "application/json", "{\"device\":\"esp32robot\",\"type\":\"car\"}");
  });

  // Iniciar el servidor WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Servidor WebSocket iniciado en el puerto 81");
  
  // Iniciar el servidor de streaming
  startCameraServer();
  
  // Hacer un parpadeo para indicar que todo está listo
  for (int i = 0; i < 3; i++) {
    turnOnFlash();
    delay(100);
    turnOffFlash();
    delay(100);
  }
  
  Serial.println("¡Sistema listo!");
  Serial.print("Stream URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":82/stream");
}

void loop() {
  webSocket.loop();
  server.handleClient();
}