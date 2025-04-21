#include <WiFi.h>

// Configuración Wi-Fi
const char* ssid = "TU_SSID";         // Cambia por el nombre de tu red Wi-Fi
const char* password = "TU_PASSWORD"; // Cambia por la contraseña de tu red Wi-Fi

// Pines L298N
#define IN1 12
#define IN2 13
#define IN3 15
#define IN4 14
#define ENA 2
#define ENB 4

WiFiServer server(80);

void setup() {
  // Configuración serial
  Serial.begin(115200);

  // Configuración de pines de salida
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Conexión Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a Wi-Fi!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Iniciar servidor
  server.begin();
  Serial.println("Servidor iniciado.");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Cliente conectado.");
    String request = client.readStringUntil('\r');
    Serial.println("Petición: " + request);

    // Control de motores
    if (request.indexOf("/linea1/avanzar") != -1) {
      linea1Avanzar();
    } else if (request.indexOf("/linea1/detener") != -1) {
      linea1Detener();
    } else if (request.indexOf("/linea2/avanzar") != -1) {
      linea2Avanzar();
    } else if (request.indexOf("/linea2/detener") != -1) {
      linea2Detener();
    }

    // Respuesta al cliente
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<html><body><h1>Comando ejecutado</h1></body></html>");
    delay(1);
    client.stop();
    Serial.println("Cliente desconectado.");
  }
}

// Funciones de control de motores
void linea1Avanzar() {
  analogWrite(ENB, 255);  // Máxima velocidad
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void linea1Detener() {
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void linea2Avanzar() {
  analogWrite(ENA, 255);  // Máxima velocidad
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void linea2Detener() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}