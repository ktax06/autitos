FROM python:3.11-slim

WORKDIR /app

# Instalar dependencias del sistema
RUN apt-get update && apt-get install -y \
    gcc openssl \
    && rm -rf /var/lib/apt/lists/*

# Generar certificados SSL
RUN openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes \
    -subj "/C=CL/ST=Maule/L=Curico/O=Utalca/CN=localhost"

# Copiar código de la aplicación
COPY . .

# Instalar dependencias de Python
RUN pip install --no-cache-dir -r requirements.txt

# Exponer puerto
EXPOSE 8888

# Comando para ejecutar la aplicación
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8888", "--ssl-keyfile", "/app/key.pem", "--ssl-certfile", "/app/cert.pem"]
