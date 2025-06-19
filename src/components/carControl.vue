<template>
  <div class="min-h-screen flex flex-col items-center bg-gray-100 p-4 space-y-4 relative">
    <!-- Título y botón de ayuda -->
    <div class="flex justify-between items-center w-full max-w-6xl">
      <h1 class="text-2xl font-bold text-black text-center w-full">Proyecto autito ICC</h1>
      <button
        @click="showHelp = true"
        class="w-10 h-10 flex items-center justify-center bg-blue-500 text-white text-lg font-bold rounded-full shadow-md hover:bg-blue-600"
        title="Ayuda e instrucciones"
        >
          ?
      </button>
    </div>

    <!-- CONTENIDO PRINCIPAL -->
    <div class="flex flex-col-reverse md:flex-row w-full max-w-6xl gap-6 mt-4 items-center md:items-start">
      <!-- CONTROLES -->
      <div class="flex flex-col items-center md:w-1/3 space-y-8 p-4 md:p-6">
        <button
          @click="toggleFlash"
          class="w-48 px-6 py-3 bg-yellow-400 hover:bg-yellow-500 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition duration-150"
          :disabled="!isConnected"
        >
          {{ flashOn ? 'Apagar Flash' : 'Encender Flash' }}
        </button>

        <!-- Joystick -->
        <div class="w-60 h-60 flex justify-center items-center">
          <JoystickComponent 
            @move="handleJoystickMove"
            @end="handleJoystickEnd"
            :disabled="!isConnected" />
        </div>

        <!-- Estado del servidor -->
        <div class="w-full text-center space-y-1">
          <p class="text-sm" :class="isConnected ? 'text-green-600' : 'text-red-600'">
            {{ isConnected ? 'WebSocket Conectado' : 'WebSocket Desconectado' }}
          </p>
          <p class="text-xs text-gray-400">{{ serverStatus }}</p>
          <button 
            v-if="!isConnected" 
            @click="connectWebSocket" 
            class="px-4 py-2 bg-blue-500 hover:bg-blue-600 text-white rounded-lg text-sm"
          >
            Reconectar
          </button>
        </div>
      </div>

      <!-- VISTA DE CÁMARA -->
      <div class="w-full md:flex-1">
        <div class="aspect-4/3 bg-black rounded-2xl overflow-hidden shadow-lg w-full">
          <img 
            v-if="cameraImageSrc" 
            :src="cameraImageSrc" 
            alt="Stream de cámara" 
            style="transform: rotate(90deg); max-width: 100%; height: auto;" 
            class="w-full h-full object-cover"
          />
          <div v-else class="flex items-center justify-center h-full text-white">
            {{ cameraErrorMessage || 'Esperando imagen de la cámara...' }}
          </div>
        </div>
        <div class="text-center mt-2 text-xs text-gray-500">
          {{ lastImageTime ? `Última imagen: ${formatTime(lastImageTime)}` : 'Sin imagen' }}
        </div>
      </div>
    </div>

    <!-- MODAL DE AYUDA -->
    <div v-if="showHelp" class="fixed inset-0 z-50 bg-black bg-opacity-50 flex items-center justify-center">
      <div class="bg-white rounded-xl shadow-xl max-w-lg w-full p-6 relative">
        <h2 class="text-xl font-semibold mb-4 text-black text-center">Instrucciones de uso</h2>
        <ul class="list-disc list-inside text-sm text-gray-700 space-y-2">
          <li>Debes crear una Zona WiFi con el nombre <strong>autoicc</strong> y la contraseña <strong>autitos1</strong>.</li>
          <li>Enciende el autito y espera a que se conecte al WiFi.</li>
          <li>Ya puedes usar la aplicación para controlar el autito.</li>
        </ul>

        <div class="mt-6 flex flex-col sm:flex-row justify-between items-center space-y-2 sm:space-y-0 sm:space-x-2">
          <a 
            :href="arduinoCodeUrl" 
            download="autito_icc.ino" 
            class="bg-green-500 hover:bg-green-600 text-white px-4 py-2 rounded-lg text-sm text-center"
          >
            Descargar código .ino
          </a>
          <button 
            @click="showDiagram = true" 
            class="bg-purple-500 hover:bg-purple-600 text-white px-4 py-2 rounded-lg text-sm"
          >
            Ver diagrama de conexión
          </button>
          <button 
            @click="showHelp = false" 
            class="text-gray-500 hover:text-black text-sm mt-2 sm:mt-0"
          >
            Cerrar
          </button>
        </div>
      </div>
    </div>

    <!-- MODAL DE DIAGRAMA -->
    <div v-if="showDiagram" class="fixed inset-0 z-50 bg-black bg-opacity-60 flex items-center justify-center">
      <div class="bg-white rounded-xl shadow-xl max-w-3xl w-full p-4 relative">
        <h2 class="text-lg font-semibold text-center mb-4">Diagrama de conexión del autito</h2>
        <img 
          :src="diagramImageUrl" 
          alt="Diagrama de conexión" 
          class="w-full rounded-lg border border-gray-200 shadow-sm"
        />
        <div class="mt-4 text-center">
          <button 
            @click="showDiagram = false" 
            class="px-4 py-2 bg-gray-300 hover:bg-gray-400 rounded text-sm"
          >
            Cerrar
          </button>
        </div>
      </div>
    </div>
  </div>
</template>


<script setup>
import JoystickComponent from './joystickComponent.vue'
import { ref, onMounted, onUnmounted } from 'vue'

// Variables reactivas
const flashOn = ref(false)
const serverStatus = ref('Iniciando conexión...')
const cameraErrorMessage = ref('')
const cameraImageSrc = ref('')
const lastImageTime = ref(null)
const isConnected = ref(false)
const showHelp = ref(false)
const showDiagram = ref(false)
const arduinoCodeUrl = 'https://api-autito.arturoalvarez.website/descargar/codigo'
const diagramImageUrl = 'https://api-autito.arturoalvarez.website/descargar/diagrama'

// WebSocket
let websocket = null
const WS_URL = 'https://api-autito.arturoalvarez.website/ws'

const lastCommandTime = ref(0)
const MIN_COMMAND_INTERVAL = 150  // milisegundos

// Función para conectar WebSocket
const connectWebSocket = () => {
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    websocket.close()
  }

  try {
    websocket = new WebSocket(WS_URL)
    
    websocket.onopen = () => {
      console.log('WebSocket conectado')
      isConnected.value = true
      serverStatus.value = 'Conectado al servidor'
      
      // Identificarse como cliente Vue
      websocket.send(JSON.stringify({
        role: 'vue'
      }))
    }

    websocket.onmessage = async (event) => {
      try {
        // Intentar parsear como JSON primero
        const data = JSON.parse(event.data)
        console.log('Mensaje recibido:', data)
        
        if (data.type === 'status') {
          serverStatus.value = `Estado: ${data.value}`
        }
      } catch (e) {
        // Si no es JSON, probablemente es una imagen (Blob)
        if (event.data instanceof Blob) {
          const imageUrl = URL.createObjectURL(event.data)
          
          // Liberar URL anterior para evitar memory leaks
          if (cameraImageSrc.value && cameraImageSrc.value.startsWith('blob:')) {
            URL.revokeObjectURL(cameraImageSrc.value)
          }
          
          cameraImageSrc.value = imageUrl
          lastImageTime.value = new Date()
          cameraErrorMessage.value = ''
        }
      }
    }

    websocket.onerror = (error) => {
      console.error('Error WebSocket:', error)
      serverStatus.value = 'Error de conexión'
      isConnected.value = false
    }

    websocket.onclose = (event) => {
      console.log('WebSocket cerrado:', event.code, event.reason)
      isConnected.value = false
      serverStatus.value = 'Conexión cerrada'
      
      // Intentar reconectar después de 3 segundos
      setTimeout(() => {
        if (!isConnected.value) {
          console.log('Intentando reconectar...')
          serverStatus.value = 'Reconectando...'
          connectWebSocket()
        }
      }, 3000)
    }

  } catch (error) {
    console.error('Error creando WebSocket:', error)
    serverStatus.value = 'Error al conectar'
    isConnected.value = false
  }
}

// Función para enviar comandos
const sendCommand = (command, params = {}) => {
  const now = Date.now()
  if (now - lastCommandTime.value < MIN_COMMAND_INTERVAL && command !== 'stop') {
    console.warn('Comando ignorado por intervalo mínimo')
    return
  }

  if (!websocket || websocket.readyState !== WebSocket.OPEN) {
    console.error('WebSocket no está conectado')
    serverStatus.value = 'Sin conexión - comando no enviado'
    return
  }

  const commandData = {
    type: 'command',
    value: command,
    ...params
  }

  try {
    websocket.send(JSON.stringify(commandData))
    lastCommandTime.value = now
    serverStatus.value = `Comando enviado: ${command}`
    console.log('Comando enviado:', commandData)
  } catch (error) {
    console.error('Error enviando comando:', error)
    serverStatus.value = 'Error enviando comando'
  }
}

// Función para toggle flash
const toggleFlash = () => {
  flashOn.value = !flashOn.value
  sendCommand(flashOn.value ? 'flash_on' : 'flash_off')
}

// Joystick
const joystickThreshold = 0.15
const joystickCommand = ref('stop')
const joystickTimer = ref(null)
const lastJoystickCommand = ref('stop')


const handleJoystickMove = (position) => {
  if (!isConnected.value) return

  const { x, y } = position
  let command = 'stop'
  let speed = 0
  const magnitude = Math.sqrt(x * x + y * y)

  // Si la magnitud es muy pequeña o cero, forzar stop
  if (magnitude < joystickThreshold) {
    command = 'stop'
    joystickCommand.value = command
    
    // Solo enviar stop si el último comando no fue stop
    if (lastJoystickCommand.value !== 'stop') {
      sendCommand(command)
      lastJoystickCommand.value = command
    }
    return
  }

  let angle = Math.atan2(y, x) * (180 / Math.PI)
  if (angle < 0) angle += 360

  // Direcciones invertidas
  if (angle >= 45 && angle < 135) {
    command = 'backward'
    speed = magnitude
  } else if (angle >= 225 && angle < 315) {
    command = 'forward'
    speed = magnitude
  } else if (angle >= 135 && angle < 225) {
    command = 'left'
    speed = magnitude
  } else if (angle >= 315 || angle < 45) {
    command = 'right'
    speed = magnitude
  }

  const newSpeed = Math.round(speed * 100)
  joystickCommand.value = command
  joystickCommand.speed = newSpeed

  sendCommand(command, { speed: newSpeed })
  lastJoystickCommand.value = command
}

const handleJoystickEnd = () => {
  if (!isConnected.value) return

  console.log('Joystick ended, sending stop command')
  sendCommand('stop', { speed: 0 })
  lastJoystickCommand.value = 'stop'
}

// Función para formatear tiempo
const formatTime = (date) => {
  return date.toLocaleTimeString()
}

// Lifecycle hooks
onMounted(() => {
  connectWebSocket()
})

onUnmounted(() => {
  if (websocket) {
    websocket.close()
  }
  
  // Limpiar URLs de blob para evitar memory leaks
  if (cameraImageSrc.value && cameraImageSrc.value.startsWith('blob:')) {
    URL.revokeObjectURL(cameraImageSrc.value)
  }
})

</script>