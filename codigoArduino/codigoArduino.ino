#include <WiFi.h>
#include <ArduinoJson.h> // Librería para manejo de JSON
#include <WebServer.h>
#include <WebSocketsServer.h>

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
  
  
  // Hacer un parpadeo para indicar que todo está listo
  for (int i = 0; i < 3; i++) {
    turnOnFlash();
    delay(100);
    turnOffFlash();
    delay(100);
  }
  
  Serial.println("¡Sistema listo!");
}

void loop() {
  webSocket.loop();
  server.handleClient();
}