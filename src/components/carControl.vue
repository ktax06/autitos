<!-- carControl.vue -->
<template>
  <div class="min-h-screen flex flex-col items-center bg-gray-100 p-4 space-y-4">
    <h1 class="text-2xl font-bold text-center">{{ currentCar.name }}</h1>

    <div class="flex flex-col-reverse md:flex-row w-full max-w-6xl gap-6 mt-4 items-center md:items-start">
      <!-- CONTROLES -->
      <div class="flex flex-col items-center md:items-start md:w-1/3 space-y-10 p-6">
        <!-- Botón Flash -->
        <button
          @click="toggleFlash"
          class="w-48 px-6 py-3 bg-yellow-400 hover:bg-yellow-500 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition-all duration-150"
        >
          {{ flashOn ? 'Apagar Flash' : 'Encender Flash' }}
        </button>

        <!-- Joystick -->
        <div class="w-40 h-40 mr-20">
          <JoystickComponent @move="handleJoystickMove" />
        </div>

        <!-- Botón Conectar Autito -->
        <button
          @click="ipModalOpen = true"
          class="w-48 px-6 py-3 mt-20 bg-indigo-600 hover:bg-indigo-700 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition-all duration-150"
        >
          Conectar Autito
        </button>
        

      </div>

      <!-- CÁMARAS -->
      <div class="w-full md:flex-1">
        <!-- Vista de cámara -->
        <div class="aspect-video bg-black rounded-2xl overflow-hidden shadow-lg w-full">
          <img 
            v-if="cameraUrl" 
            :src="cameraUrl" 
            alt="Cámara" 
            class="w-full h-full object-cover" 
          />
          <div v-else class="flex items-center justify-center h-full text-white">
            Esperando señal...
          </div>
        </div>
      </div>
    </div>

    <!-- MODAL Conexión IP -->
    <div v-if="ipModalOpen" class="fixed inset-0 bg-black/60 backdrop-blur-sm flex items-center justify-center z-50">
      <div class="text-black bg-white p-6 rounded-2xl shadow-xl w-96 space-y-4">
        <h2 class="text-xl font-bold text-center">Conectar Autito</h2>
        
        <label class="block">
          <span class="text-sm font-medium text-gray-700">Dirección IP del autito</span>
          <input
            v-model="manualIp"
            :class="[
              'mt-1 w-full px-4 py-2 border rounded-lg shadow-sm focus:outline-none focus:ring text-black',
              ipIsValid ? 'border-gray-300 focus:ring-indigo-200' : 'border-red-500 focus:ring-red-200'
            ]"
            placeholder="ej: 192.168.1.123"
          />
        </label>
        

        <!-- Botones -->
        <div class="flex justify-end gap-4">
          
          <button
            @click="() => connectToESP()"
            :class="[
              'px-4 py-2 rounded-lg font-semibold shadow-md bg-indigo-500 hover:bg-indigo-600 text-white'
            ]"
            >
            Conectar
          </button>
          <button
            @click="ipModalOpen = false"
            :class="[
              'px-4 py-2 rounded-lg font-semibold shadow-md bg-gray-300 hover:bg-gray-400 text-gray-800 '
            ]"
            >
            Cancelar
          </button>
        </div>


        <div v-if="connectionStatus" class="text-center text-sm text-gray-700">
          Estado: {{ connectionStatus }}
        </div>
      </div>
    </div>

    <!-- Componente WebSocket (invisible) -->
    <ESP32WebSocket ref="webSocketComponent" />
    
  </div>
</template>

<script setup>
import JoystickComponent from './joystickComponent.vue'
import { onMounted, ref, onBeforeUnmount} from 'vue'
import ESP32WebSocket from './websocket.vue'

const flashOn = ref(false)
const modalOpen = ref(false)
const ipModalOpen = ref(false)
const cameraUrl = ref('') // URL de la cámara (puerto 82)
const manualIp = ref('')
const connectionStatus = ref('')
const webSocketComponent = ref(null)

const currentCar = ref({ id: 1, name: 'Proyecto autito ICC' }) 
const availableCars = ref([]) //rellénalo desde API o backend

const toggleFlash = () => {
  flashOn.value = !flashOn.value
  sendCommand(flashOn.value ? 'FLASH_ON' : 'FLASH_OFF')
}

const selectCar = (car) => {
  currentCar.value = car
  modalOpen.value = false
}
const ipIsValid = ref(true)

const connectToESP = async (ip = null) => {
  // Usar la IP proporcionada o la IP manual del input
  const targetIp = ip || manualIp.value
  
  // Validar que targetIp sea un string válido
  if (!targetIp || typeof targetIp !== 'string') {
    connectionStatus.value = 'IP no proporcionada o inválida'
    return
  }
  
  if (!isValidIp(manualIp.value)) {
    ipIsValid.value = false
    connectionStatus.value = 'IP inválida. Formato esperado: 192.168.X.X'
    return
  }

  try {
    connectionStatus.value = 'Conectando...'
    
    // Desconectar el WebSocket existente
    if (webSocketComponent.value) {
      webSocketComponent.value.disconnectWebSocket()
    }
    
    // Actualizar URL de la cámara en el puerto 82
    cameraUrl.value = `http://${targetIp}:82/stream`
    
    // Verificar conexión a la cámara
    try {
      const response = await fetch(`http://${targetIp}:82/`, { mode: 'no-cors' })
      console.log('Cámara disponible en puerto 82')
    } catch (err) {
      console.warn('No se pudo conectar a la cámara en puerto 82:', err)
      cameraUrl.value = ''
    }
    
     // Crear una nueva conexión WebSocket con el componente
    console.log(`Conectando WebSocket a: ws://${targetIp}:81/`)
    webSocketComponent.value.connectWebSocket(`ws://${targetIp}:81/`)
    
    connectionStatus.value = 'Conectado'
    ipModalOpen.value = false

  } catch (err) {
    connectionStatus.value = 'Error al conectar'
    console.error('Error en connectToESP:', err)
  }
}

const sendCommand = (cmd, params = {}) => {
  if (webSocketComponent.value) {
    webSocketComponent.value.sendMessage(JSON.stringify({ 
      type: 'command', 
      value: cmd,
      ...params
    }))
  } else {
    console.warn('Componente WebSocket no inicializado')
  }
}

// Manejo del joystick
const joystickThreshold = 0.15; // Umbral para evitar movimientos mínimos
const joystickCommand = ref('STOP');
const joystickTimer = ref(null);

const handleJoystickMove = (position) => {
  // Detener el temporizador anterior si existe
  if (joystickTimer.value) {
    clearTimeout(joystickTimer.value);
  }
  
  // Calcular la dirección basada en la posición del joystick
  const { x, y } = position;
  let command = 'STOP';
  let speed = 0;
  let turn = 0;
  
  // Calcular magnitud (0-1)
  const magnitude = Math.sqrt(x * x + y * y);
  
  // Si la magnitud es menor que el umbral, consideramos que está en reposo
  if (magnitude < joystickThreshold) {
    command = 'STOP';
  } else {
    // Calcular el ángulo en grados (0-360)
    let angle = Math.atan2(y, x) * (180 / Math.PI);
    if (angle < 0) angle += 360;
    
    // Determinar comando basado en el ángulo
    if (angle >= 315 || angle < 45) {
      command = 'RIGHT';
      turn = magnitude;
    } else if (angle >= 45 && angle < 135) {
      command = 'FORWARD';
      speed = magnitude;
    } else if (angle >= 135 && angle < 225) {
      command = 'LEFT';
      turn = magnitude;
    } else if (angle >= 225 && angle < 315) {
      command = 'BACKWARD';
      speed = magnitude;
    }
  }
  
  // Solo enviar si el comando ha cambiado o si los valores son significativamente diferentes
  if (command !== joystickCommand.value || (magnitude > joystickThreshold)) {
    joystickCommand.value = command;
    
    // Enviar el comando con los parámetros de velocidad y giro
    sendCommand(command, { 
      speed: Math.round(speed * 100), 
      turn: Math.round(turn * 100)
    });
    
    // Configurar un temporizador para enviar STOP si no hay movimiento durante un tiempo
    if (command !== 'STOP') {
      joystickTimer.value = setTimeout(() => {
        sendCommand('STOP');
        joystickCommand.value = 'STOP';
      }, 200);
    }
  }
}

// Comprobar disponibilidad de la cámara periódicamente
const checkCameraStatus = () => {
  if (!manualIp.value || !isValidIp(manualIp.value)) {
    console.warn('Dirección IP no válida:', manualIp.value);
    return;
  }

  // Comprobar cámara en puerto 82
  fetch(`http://${manualIp.value}:82/`, { mode: 'no-cors' })
    .then(() => {
      // Asegurarnos de que la URL está correctamente configurada si la cámara está disponible
      if (!cameraUrl.value) {
        cameraUrl.value = `http://${manualIp.value}:82/stream`;
      }
    })
    .catch(() => {
      cameraUrl.value = '';
      console.warn('La cámara en el puerto 82 no está respondiendo');
    });
};

// Comprobar cada 10 segundos

const cameraCheckInterval = ref(null);

onMounted(() => {
  // Iniciar el intervalo SOLO cuando el componente se monte en el cliente
  cameraCheckInterval.value = setInterval(checkCameraStatus, 10000);
});

onBeforeUnmount(() => {
  if (joystickTimer.value) clearTimeout(joystickTimer.value)
  if (cameraCheckInterval.value) clearInterval(cameraCheckInterval.value)
  if (webSocketComponent.value) webSocketComponent.value.disconnectWebSocket()
})

const isValidIp = (ip) => {
  const ipRegex = /^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$/;
  return ipRegex.test(ip);
};
</script>