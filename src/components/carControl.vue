<template>
  <div class="min-h-screen flex flex-col items-center bg-gray-100 p-4 space-y-4">
    <h1 class="text-2xl font-bold text-center text-black">Proyecto autito ICC</h1>

    <div class="flex flex-col-reverse md:flex-row w-full max-w-6xl gap-6 mt-4 items-center md:items-start">
      <!-- CONTROLES -->
      <div class="flex flex-col items-center md:w-1/3 space-y-8 p-4 md:p-6">
        <!-- Botón Flash -->
        <button
          @click="toggleFlash"
          class="w-48 px-6 py-3 bg-yellow-400 hover:bg-yellow-500 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition duration-150"
        >
          {{ flashOn ? 'Apagar Flash' : 'Encender Flash' }}
        </button>

        <!-- Joystick -->
        <div class="w-60 h-60 flex justify-center items-center">
          <JoystickComponent @move="handleJoystickMove" />
        </div>

        <!-- Estado del servidor -->
        <div class="w-full text-center space-y-1">
          <p class="text-sm text-gray-500">Modo HTTP</p>
          <p class="text-xs text-gray-400">{{ serverStatus }}</p>
        </div>
      </div>

      <!-- VISTA DE CÁMARA -->
      <div class="w-full md:flex-1">
        <!-- Vista de cámara -->
        <div class="aspect-4/3 bg-black rounded-2xl overflow-hidden shadow-lg w-full">
          <img 
            v-if="cameraUrl" 
            :src="cameraUrl" 
            alt="Stream de cámara" 
            @error="handleCameraError"
            style="transform: rotate(90deg); max-width: 100%; height: auto;" />
          <div v-else class="flex items-center justify-center h-full text-white">
            {{ cameraErrorMessage || 'Cargando cámara...' }}
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import JoystickComponent from './joystickComponent.vue'
import { ref, computed, onMounted, onUnmounted } from 'vue'


// Variables reactivas
const flashOn = ref(false)
const serverStatus = ref('')
const cameraErrorMessage = ref('')

// API base
const API_BASE_URL = 'http://localhost:8000'

const cameraTimestamp = ref(Date.now())

const cameraUrl = computed(() => `${API_BASE_URL}/camera?t=${cameraTimestamp.value}`)

let cameraInterval = null

onMounted(() => {
  cameraInterval = setInterval(() => {
    cameraTimestamp.value = Date.now()
  }, 500) // cada 1 segundo
})

onUnmounted(() => {
  clearInterval(cameraInterval)
})


const apiCall = async (endpoint, options = {}) => {
  try {
    const response = await fetch(`${API_BASE_URL}${endpoint}`, {
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      },
      ...options
    })

    if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`)

    return await response.json()
  } catch (error) {
    console.error('API call error:', error)
    serverStatus.value = `Error API: ${error.message}`
    throw error
  }
}

const sendCommand = async (command, params = {}) => {
  const commandData = {
    type: 'command',
    value: command,
    ...params
  }

  try {
    await apiCall('/robots/command', {
      method: 'POST',
      body: JSON.stringify(commandData)
    })
    serverStatus.value = `Comando enviado: ${command}`
  } catch (error) {
    console.error('Error enviando comando:', error)
    serverStatus.value = 'Fallo al enviar comando'
  }
}

const toggleFlash = () => {
  flashOn.value = !flashOn.value
  sendCommand(flashOn.value ? 'flash_on' : 'flash_off')
}

// Joystick
const joystickThreshold = 0.15
const joystickCommand = ref('stop')
const joystickTimer = ref(null)

const handleJoystickMove = (position) => {
  if (joystickTimer.value) clearTimeout(joystickTimer.value)

  const { x, y } = position
  let command = 'stop'
  let speed = 0
  let turn = 0

  const magnitude = Math.sqrt(x * x + y * y)

  if (magnitude < joystickThreshold) {
    command = 'stop'
  } else {
    let angle = Math.atan2(y, x) * (180 / Math.PI)
    if (angle < 0) angle += 360

    if (angle >= 315 || angle < 45) {
      command = 'right'
      turn = magnitude
    } else if (angle >= 45 && angle < 135) {
      command = 'forward'
      speed = magnitude
    } else if (angle >= 135 && angle < 225) {
      command = 'left'
      turn = magnitude
    } else if (angle >= 225 && angle < 315) {
      command = 'backward'
      speed = magnitude
    }
  }

  if (command !== joystickCommand.value || magnitude > joystickThreshold) {
    joystickCommand.value = command

    sendCommand(command, { 
      speed: Math.round(speed * 100), 
      turn: Math.round(turn * 100)
    })

    if (command !== 'stop') {
      joystickTimer.value = setTimeout(() => {
        sendCommand('stop')
        joystickCommand.value = 'stop'
      }, 200)
    }
  }
}

const handleCameraError = () => {
  console.warn('Error cargando stream de cámara')
  cameraErrorMessage.value = 'No se pudo cargar la cámara'
}

</script>