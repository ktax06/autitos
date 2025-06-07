from fastapi import FastAPI, HTTPException, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import StreamingResponse, JSONResponse
from pydantic import BaseModel
from typing import Optional, Dict, Any
import httpx
import asyncio
import logging
from datetime import datetime
import io

# Configuración de logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(
    title="Robot Control API",
    description="API intermediaria para controlar robot ESP32-CAM",
    version="1.0.0"
)

# Configuración CORS para Vue.js
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://localhost:8080", "http://127.0.0.1:3000", "http://127.0.0.1:8080", "http://localhost:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Configuración del ESP32-CAM
ESP32_IP = "192.168.1.26"  # Cambia por la IP de tu ESP32-CAM
ESP32_PORT = 80
ESP32_BASE_URL = f"http://{ESP32_IP}:{ESP32_PORT}"

# Modelos Pydantic
class RobotCommand(BaseModel):
    type: str = "command"
    value: str
    speed: Optional[int] = 0
    turn: Optional[int] = 0
    duration: Optional[int] = None

class CommandResponse(BaseModel):
    success: bool
    message: str
    timestamp: str
    command: str

# Estado global del robot
robot_state = {
    "connected": False,
    "last_command": "STOP",
    "flash_on": False,
    "last_ping": None
}

# Cliente HTTP para comunicación con ESP32
async def get_esp32_client():
    return httpx.AsyncClient(timeout=httpx.Timeout(5.0))

# Funciones de comunicación con ESP32-CAM
async def send_to_esp32(endpoint: str, params: Dict[str, Any] = None) -> Dict[str, Any]:
    """Envía comandos al ESP32-CAM"""
    try:
        async with await get_esp32_client() as client:
            url = f"{ESP32_BASE_URL}{endpoint}"
            
            if params:
                response = await client.get(url, params=params)
            else:
                response = await client.get(url)
            
            response.raise_for_status()
            robot_state["connected"] = True
            robot_state["last_ping"] = datetime.now().isoformat()
            
            return {"success": True, "data": response.text}
            
    except Exception as e:
        logger.error(f"Error comunicando con ESP32: {e}")
        robot_state["connected"] = False
        return {"success": False, "error": str(e)}

# Mapeo de comandos a endpoints del ESP32
COMMAND_MAPPING = {
    "FORWARD": "/forward",
    "BACKWARD": "/backward", 
    "LEFT": "/left",
    "RIGHT": "/right",
    "STOP": "/stop",
    "FLASH_ON": "/flash/on",
    "FLASH_OFF": "/flash/off"
}

@app.get("/")
async def root():
    """Endpoint de salud de la API"""
    return {
        "message": "Robot Control API",
        "status": "running",
        "esp32_connected": robot_state["connected"],
        "version": "1.0.0"
    }

@app.get("/status")
async def get_status():
    """Obtiene el estado actual del robot"""
    # Hacer ping al ESP32 para verificar conexión
    ping_result = await send_to_esp32("/status")
    
    return {
        "robot_state": robot_state,
        "esp32_response": ping_result,
        "timestamp": datetime.now().isoformat()
    }

@app.post("/robots/command")
async def send_robot_command(command: RobotCommand) -> CommandResponse:
    """Envía comandos al robot ESP32-CAM"""
    try:
        command_value = command.value.upper()
        
        if command_value not in COMMAND_MAPPING:
            raise HTTPException(
                status_code=400, 
                detail=f"Comando no válido: {command_value}. Comandos válidos: {list(COMMAND_MAPPING.keys())}"
            )
        
        endpoint = COMMAND_MAPPING[command_value]
        params = {}
        
        # Agregar parámetros según el comando
        if command_value in ["FORWARD", "BACKWARD"] and command.speed:
            params["speed"] = min(max(command.speed, 0), 100)  # Limitar entre 0-100
            
        if command_value in ["LEFT", "RIGHT"] and command.turn:
            params["turn"] = min(max(command.turn, 0), 100)  # Limitar entre 0-100
            
        if command.duration:
            params["duration"] = command.duration
        
        # Enviar comando al ESP32
        result = await send_to_esp32(endpoint, params if params else None)
        
        if result["success"]:
            robot_state["last_command"] = command_value
            if command_value in ["FLASH_ON", "FLASH_OFF"]:
                robot_state["flash_on"] = command_value == "FLASH_ON"
            
            return CommandResponse(
                success=True,
                message=f"Comando {command_value} enviado exitosamente",
                timestamp=datetime.now().isoformat(),
                command=command_value
            )
        else:
            raise HTTPException(
                status_code=503,
                detail=f"Error comunicando con ESP32: {result['error']}"
            )
            
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Error procesando comando: {e}")
        raise HTTPException(status_code=500, detail=f"Error interno: {str(e)}")

@app.get("/camera")
async def get_camera_stream():
    """Proxy del stream de cámara del ESP32-CAM"""
    try:
        async with await get_esp32_client() as client:
            camera_url = f"{ESP32_BASE_URL}/cam-hi.jpg"  # Endpoint típico de ESP32-CAM
            
            async with client.stream("GET", camera_url) as response:
                response.raise_for_status()
                
                async def generate():
                    async for chunk in response.aiter_bytes():
                        yield chunk
                
                return StreamingResponse(
                    generate(),
                    media_type="image/jpeg",
                    headers={
                        "Cache-Control": "no-cache, no-store, must-revalidate",
                        "Pragma": "no-cache",
                        "Expires": "0"
                    }
                )
                
    except Exception as e:
        logger.error(f"Error obteniendo stream de cámara: {e}")
        raise HTTPException(
            status_code=503,
            detail=f"No se pudo obtener el stream de cámara: {str(e)}"
        )

@app.get("/camera/snapshot")
async def get_camera_snapshot():
    """Obtiene una foto individual de la cámara"""
    try:
        result = await send_to_esp32("/capture")
        
        if result["success"]:
            return JSONResponse({
                "success": True,
                "message": "Foto capturada",
                "timestamp": datetime.now().isoformat()
            })
        else:
            raise HTTPException(status_code=503, detail="Error capturando foto")
            
    except Exception as e:
        logger.error(f"Error capturando foto: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/emergency-stop")
async def emergency_stop():
    """Parada de emergencia del robot"""
    try:
        result = await send_to_esp32("/stop")
        robot_state["last_command"] = "EMERGENCY_STOP"
        
        return {
            "success": True,
            "message": "Parada de emergencia activada",
            "timestamp": datetime.now().isoformat()
        }
        
    except Exception as e:
        logger.error(f"Error en parada de emergencia: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# Configuración de endpoints del ESP32-CAM
@app.put("/config/esp32")
async def update_esp32_config(ip: str, port: int = 80):
    """Actualiza la configuración de conexión del ESP32-CAM"""
    global ESP32_IP, ESP32_PORT, ESP32_BASE_URL
    
    try:
        # Validar formato IP básico
        parts = ip.split('.')
        if len(parts) != 4 or not all(0 <= int(part) <= 255 for part in parts):
            raise ValueError("Formato de IP inválido")
        
        ESP32_IP = ip
        ESP32_PORT = port
        ESP32_BASE_URL = f"http://{ESP32_IP}:{ESP32_PORT}"
        
        # Probar conexión
        test_result = await send_to_esp32("/status")
        
        return {
            "success": True,
            "message": f"Configuración actualizada: {ESP32_BASE_URL}",
            "connection_test": test_result,
            "timestamp": datetime.now().isoformat()
        }
        
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Error actualizando configuración: {str(e)}")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "main:app",
        host="0.0.0.0",
        port=8000,
        reload=True,
        log_level="info"
    )