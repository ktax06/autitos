#include <WiFi.h>

// --- CONFIGURACIÓN WIFI ---
const char* ssid = "TU_SSID";         // <-- Cambiar esto
const char* password = "TU_PASSWORD"; // <-- Cambiar esto

WiFiServer server(80);

// --- PINES PARA L298N ---
#define IN1 12
#define IN2 13
#define IN3 15
#define IN4 14
#define ENA 2   // Línea 2
#define ENB 4   // Línea 1

void setup() {
  Serial.begin(115200);

  // Configurar pines como salida
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Conectar al Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n Conectado a Wi-Fi!");
  Serial.print("🔗 IP: ");
  Serial.println(WiFi.localIP());

  // Iniciar servidor web
  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("📲 Cliente conectado");

    String request = client.readStringUntil('\r');
    Serial.println("➡️ Petición: " + request);

    // --- CONTROL DE MOTORES SEGÚN PETICIÓN ---
    if (request.indexOf("/linea1/avanzar") != -1) {
      linea1Avanzar();
    } else if (request.indexOf("/linea1/detener") != -1) {
      linea1Detener();
    } else if (request.indexOf("/linea2/avanzar") != -1) {
      linea2Avanzar();
    } else if (request.indexOf("/linea2/detener") != -1) {
      linea2Detener();
    }

    // --- RESPUESTA HTTP ---
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<html><body><h1> Comando ejecutado</h1></body></html>");

    delay(10);
    client.stop();
    Serial.println("🔌 Cliente desconectado\n");
  }
}

// --- FUNCIONES DE CONTROL DE MOTORES ---

void linea1Avanzar() {
  analogWrite(ENB, 255);       // ENB controla velocidad de Línea 1
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void linea1Detener() {
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void linea2Avanzar() {
  analogWrite(ENA, 255);       // ENA controla velocidad de Línea 2
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void linea2Detener() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}