# Documentación de despliegue

## Clonar repositorio

```bash
git clone https://github.com/ktax06/autitos.git
cd autitos
```

## Construir imágenes

Cambia las siguientes URLs en el archivo `app/src/components/carControl.vue` por las URLs de tu servidor:

```javascript
const arduinoCodeUrl = 'https://api-autito.tu-dominio.com/descargar/codigo'
const diagramImageUrl = 'https://api-autito.tu-dominio.com/descargar/diagrama'
const WS_URL = 'https://api-autito.tu-dominio.com/ws'
```

```bash
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t tu-cuenta-de-docker/app-autitos:latest \
  --push .

cd api

docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t tu-cuenta-de-docker/api-autitos:latest \
  --push .
```

## Desplegar en la maquina

### Crear docker-compose.yml

```yaml
services:
  app-autitos:
    image: arturoalvarez/app-autitos:latest
    container_name: APP-AUTITOS
    ports:
      - "3000:80"
    networks:
      - autitos
    restart: unless-stopped
  
  api-autitos:
    image: arturoalvarez/api-autitos:latest
    container_name: API-AUTITOS
    ports:
      - "8888:8888"
    networks:
      - autitos
    restart: unless-stopped
  
  watchtower:
    image: containrrr/watchtower
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock

networks:
  autitos:
    driver: bridge
```

### Desplegar con Docker Compose

```bash
docker compose up -d
```

## Configurar Túnel

> Crear cuenta en Cloudflare si aun no posees una.

### Instalar Cloudflare Tunnel

- Ingresar a Zero Trust

![Zero Trust](./deploy-images/zerotrust.png)

- En la sección **Networks** seleccionar **Tunnels** y crear un túnel

![Tunnels](./deploy-images/tunnels.png)

- Seleccionar **Create a tunnel**

![create-tunnel](./deploy-images/crear-tunel.png)

- Seleccionar el túnel de tipo **Cloudflared**

![cloudflared](./deploy-images/cloudflared.png)

- Darle un nombre al túnel y seleccionar **Save tunnel**

- Seleccionar el sistema operativo de la máquina donde se ejecutará el túnel y ejecutar el comando que se muestra en la terminal en la máquina donde se ejecutará el túnel

### Configurar el túnel

- En la sección **Public Hostname** seleccionar **Add public hostname**

![public-hostname](./deploy-images/public-hostname.png)

#### Configuración para el servidor web

- Subdomain: `autitos`
- Domain: `tu-dominio.com`
- path: `En blanco`
- Service: `http://localhost:3000`

> En el momento que ingreses el dominio, Cloudflare te guiara para que lo apuntes a los servidores de Cloudflare. Una vez que lo hayas hecho, podrás continuar con la configuración.

Guardar los cambios.

![servidor-web](./deploy-images/servidor-web.png)

#### Configuración para la API

- Subdomain: `api-autitos`
- Domain: `tu-dominio.com`
- path: `En blanco`
- Service: `https://localhost:8888`

![servidor-api](./deploy-images/servidor-api.png)

Configuración adicional:

- En la sección **Additional application settings** seleccionar **TLS** y activar la opción **No TLS verify**
Guardar los cambios.

![additional-config](./deploy-images/additional-config.png)

En este punto ya deberías poder acceder a tu aplicación web y API a través de los subdominios configurados.
