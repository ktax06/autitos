import express from "express";
import http from "http";
import { WebSocketServer } from "ws";

const app = express();
const server = http.createServer(app);

const wss = new WebSocketServer({ server, path: "/ws" });
let clients = [];

wss.on("connection", (ws) => {
  console.log("Cliente conectado");
  clients.push(ws);

  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message);

      if (data.type === "image") {
        // Reenviar imagen a todos los clientes
        clients.forEach((client) => {
          if (client.readyState === 1) {
            client.send(JSON.stringify({ type: "image", value: data.value }));
          }
        });
      } else if (data.type === "command") {
        // Enviar comando al ESP32 (o imprimir)
        console.log("Comando:", data.value);
        // Aquí podrías reenviarlo al ESP32 si está conectado
      }
    } catch (err) {
      console.error("Error al procesar mensaje:", err);
    }
  });

  ws.on("close", () => {
    console.log("Cliente desconectado");
    clients = clients.filter((c) => c !== ws);
  });
});

const PORT = 3000;
server.listen(PORT, () => {
  console.log(`Servidor WebSocket en puerto ${PORT}`);
});
