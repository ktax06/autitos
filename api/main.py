from fastapi import FastAPI, HTTPException, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import Response, StreamingResponse
from pydantic import BaseModel
from typing import Optional
import asyncio
import time
from datetime import datetime
import logging

# Configurar logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(title="ESP32 Car Control API", version="1.0.0")

# Configurar CORS para permitir requests desde Vue
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # En producción, especifica dominios específicos
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Modelos Pydantic
class CommandRequest(BaseModel):
    type: str = "command"
    value: str

class ActionResponse(BaseModel):
    action: str

# Variables globales para manejo de estado
current_action = "stop"
action_lock = asyncio.Lock()
latest_image: Optional[bytes] = None
image_lock = asyncio.Lock()
last_image_time = 0

@app.get("/")
async def root():
    """Endpoint de prueba"""
    return {
        "message": "ESP32 Car Control API",
        "status": "running",
        "timestamp": datetime.now().isoformat()
    }

@app.get("/health")
async def health_check():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "current_action": current_action,
        "last_image_time": last_image_time,
        "timestamp": datetime.now().isoformat()
    }

# ENDPOINTS PARA COMUNICACIÓN CON VUE

@app.post("/robots/command")
async def send_command(command: CommandRequest):
    """
    Recibe comandos desde la app Vue y los almacena para el ESP32
    """
    global current_action
    
    try:
        # Mapear comando de Vue a ESP32
        esp32_action = command.value
        
        if not esp32_action:
            raise HTTPException(
                status_code=400, 
                detail=f"Comando desconocido: {command.value}"
            )
        
        async with action_lock:
            current_action = esp32_action
            
        logger.info(f"Comando recibido: {command.value} -> {esp32_action}")
        
        return {
            "status": "success",
            "command": command.value,
            "esp32_action": esp32_action,
            "timestamp": datetime.now().isoformat()
        }
        
    except Exception as e:
        logger.error(f"Error procesando comando: {str(e)}")
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/camera")
async def get_camera_stream():
    """
    Sirve la última imagen de la cámara para Vue
    """
    global latest_image
    
    async with image_lock:
        if latest_image is None:
            # Retornar imagen placeholder si no hay imagen
            return Response(
                content=b"",
                media_type="image/jpeg",
                headers={"Cache-Control": "no-cache"}
            )
        
        return Response(
            content=latest_image,
            media_type="image/jpeg",
            headers={
                "Cache-Control": "no-cache",
                "Access-Control-Allow-Origin": "*"
            }
        )

# ENDPOINTS PARA COMUNICACIÓN CON ESP32

@app.get("/esp32/action")
async def get_esp32_action():
    """
    Endpoint que consulta el ESP32 para obtener la acción actual
    """
    global current_action
    
    async with action_lock:
        action_to_send = current_action
        # Opcional: resetear a stop después de enviar (depende del comportamiento deseado)
        # current_action = "stop"
    
    logger.info(f"ESP32 solicitó acción: {action_to_send}")
    
    return {"action": action_to_send}

@app.post("/esp32/upload_image")
async def upload_esp32_image(request: Request):
    """
    Recibe imágenes del ESP32 y las almacena para servir a Vue
    """
    global latest_image, last_image_time
    
    try:
        # Leer el contenido de la imagen
        image_data = await request.body()
        
        if not image_data:
            raise HTTPException(status_code=400, detail="No se recibió imagen")
        
        async with image_lock:
            latest_image = image_data
            last_image_time = time.time()
        
        logger.info(f"Imagen recibida del ESP32: {len(image_data)} bytes")
        
        return {
            "status": "success",
            "image_size": len(image_data),
            "timestamp": datetime.now().isoformat()
        }
        
    except Exception as e:
        logger.error(f"Error recibiendo imagen: {str(e)}")
        raise HTTPException(status_code=500, detail=str(e))

# ENDPOINTS ADICIONALES DE UTILIDAD

@app.get("/status")
async def get_system_status():
    """
    Retorna el estado actual del sistema
    """
    return {
        "current_action": current_action,
        "has_image": latest_image is not None,
        "last_image_time": last_image_time,
        "image_age_seconds": time.time() - last_image_time if last_image_time > 0 else None,
        "timestamp": datetime.now().isoformat()
    }

@app.post("/reset")
async def reset_system():
    """
    Resetea el sistema a estado inicial
    """
    global current_action, latest_image, last_image_time
    
    async with action_lock:
        current_action = "stop"
    
    async with image_lock:
        latest_image = None
        last_image_time = 0
    
    logger.info("Sistema reseteado")
    
    return {
        "status": "success",
        "message": "Sistema reseteado",
        "timestamp": datetime.now().isoformat()
    }

# Tarea en background para limpiar imágenes viejas (opcional)
@app.on_event("startup")
async def startup_event():
    """
    Tareas de inicialización
    """
    logger.info("API iniciada correctamente")
    
    # Opcional: crear tarea para limpiar imágenes viejas
    # asyncio.create_task(cleanup_old_images())

async def cleanup_old_images():
    """
    Limpia imágenes que son demasiado viejas
    """
    global latest_image, last_image_time
    
    while True:
        await asyncio.sleep(30)  # Revisar cada 30 segundos
        
        if last_image_time > 0 and time.time() - last_image_time > 10:  # 10 segundos
            async with image_lock:
                latest_image = None
                last_image_time = 0
            logger.info("Imagen antigua eliminada")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8888, log_level="info")