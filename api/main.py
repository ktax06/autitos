from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import Response, FileResponse
from datetime import datetime
from typing import Optional
import asyncio
import time
import logging
import os

# Logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    datefmt="%H:%M:%S"
)
logger = logging.getLogger("WebSocketServer")

# FastAPI app
app = FastAPI(title="ESP32 Car Control API", version="2.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Cambia esto en producción
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"]
)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Estado compartido
clients_vue = []
clients_esp32 = []

current_action = "stop"
action_lock = asyncio.Lock()

latest_image: Optional[bytes] = None
image_lock = asyncio.Lock()
last_image_time = 0

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    logger.info("Cliente conectado")
    
    try:
        # Identificación inicial
        ident = await websocket.receive_json()
        role = ident.get("role")
        if role == "esp32":
            clients_esp32.append(websocket)
            logger.info("ESP32 conectado")
        else:
            clients_vue.append(websocket)
            logger.info("Cliente Vue conectado")

        while True:
            data = await websocket.receive_json()
            if data["type"] == "command":
                # Cliente Vue envía comando con tipo y valor (y ahora también velocidad)
                async with action_lock:
                    global current_action
                    current_action = data["value"]

                    # Si viene velocidad, la usamos; si no, usamos un valor por defecto
                    velocidad = data.get("speed", 255)  # máximo por defecto

                logger.info(f"Comando recibido: {current_action} con velocidad {velocidad}")

                # Enviar comando a todos los ESP32 conectados
                for client in clients_esp32[:]:  # [:] para hacer una copia y evitar modificar mientras iteras
                    try:
                        await client.send_json({
                            "type": "action",
                            "value": current_action,
                            "speed": velocidad
                        })
                    except Exception as e:
                        logger.warning(f"ESP32 desconectado, eliminando de la lista. Error: {e}")
                        clients_esp32.remove(client)

            elif data["type"] == "image":
                # ESP32 envía imagen en base64 o bytes (recomendada: bytes con .receive_bytes())
                image_bytes = await websocket.receive_bytes()
                async with image_lock:
                    global latest_image, last_image_time
                    latest_image = image_bytes
                    last_image_time = time.time()

                # logger.info(f"Imagen recibida ({len(image_bytes)} bytes)")

                for vue in clients_vue:
                    try:
                        await vue.send_bytes(image_bytes)
                    except Exception as e:
                        logger.warning(f"Cliente Vue desconectado, eliminando de la lista. Error: {e}")
                        clients_vue.remove(vue)

            else:
                logger.warning(f"Tipo desconocido: {data['type']}")

    except WebSocketDisconnect:
        logger.info("Cliente desconectado")
        if websocket in clients_vue:
            clients_vue.remove(websocket)
        if websocket in clients_esp32:
            clients_esp32.remove(websocket)

@app.get("/camera")
async def get_latest_image():
    async with image_lock:
        if latest_image:
            return Response(
                content=latest_image,
                media_type="image/jpeg",
                headers={
                    "Cache-Control": "no-cache",
                    "Access-Control-Allow-Origin": "*"
                }
            )
        return Response(content=b"", media_type="image/jpeg")

@app.get("/")
async def root():
    return {
        "message": "WebSocket-based ESP32 Car Control API",
        "status": "running",
        "timestamp": datetime.now().isoformat()
    }

@app.get("/descargar/codigo", response_class=FileResponse)
async def descargar_codigo():
    filepath = os.path.join(BASE_DIR, "codigoESP32.ino")
    return FileResponse(filepath, filename="codigoESP32.ino", media_type="text/plain")

@app.get("/descargar/diagrama", response_class=FileResponse)
async def descargar_diagrama():
    filepath = os.path.join(BASE_DIR,"diagrama_de_conexion.png")
    return FileResponse(filepath, filename="diagrama_de_conexion.png", media_type="image/png")

@app.get("/status")
async def get_status():
    return {
        "current_action": current_action,
        "has_image": latest_image is not None,
        "last_image_time": last_image_time,
        "image_age": time.time() - last_image_time if last_image_time else None,
        "timestamp": datetime.now().isoformat()
    }

@app.on_event("startup")
async def startup_event():
    logger.info("Servidor WebSocket iniciado")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8888, log_level="info")