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
      <div class="bg-white p-6 rounded-2xl shadow-xl w-96 space-y-4">
        <h2 class="text-xl font-bold text-center">Conectar Autito</h2>
        
        <label class="block">
          <span class="text-sm font-medium text-gray-700">Dirección IP del autito</span>
          <input
          v-model="manualIp"
          :class="[
            'mt-1 w-full px-4 py-2 border rounded-lg shadow-sm focus:outline-none focus:ring',
            ipIsValid ? 'border-gray-300 focus:ring-indigo-200' : 'border-red-500 focus:ring-red-200'
          ]"
          placeholder="ej: 192.168.1.123"
        />
        </label>

        <!-- Resultados búsqueda -->
        <div v-if="searching" class="text-sm text-gray-500 text-center">
          Buscando dispositivos en red...
        </div>
       <!-- Indicador de carga -->
        <div class="flex justify-center mt-2" v-if="searching">
          <div class="w-8 h-8 border-4 border-blue-500 border-t-transparent rounded-full animate-spin"></div>
        </div>

        

        <!-- Botones -->
        <div class="flex justify-end gap-4">
          
          <button
            @click="connectToESP"
            :class="[
              'px-4 py-2 rounded-lg font-semibold shadow-md',
              searching
                ? 'bg-gray-400 text-white cursor-not-allowed'
                : 'bg-indigo-500 hover:bg-indigo-600 text-white'
            ]"
            >
            Conectar
          </button>
          <button
            @click="ipModalOpen = false"
            :disabled="searching"
            :class="[
              'px-4 py-2 rounded-lg font-semibold shadow-md',
              searching
                ? 'bg-gray-400 text-white cursor-not-allowed'
                : 'bg-gray-300 hover:bg-gray-400 text-gray-800 '
            ]"
            >
            Cancelar
          </button>
        </div>

         <ul v-if="foundDevices.length" class="text-sm mt-2 max-h-32 overflow-y-auto">
          <li v-for="ip in foundDevices" :key="ip">
            <button
              @click="connectToESP(ip)"
              class="w-full text-left px-3 py-1 hover:bg-gray-200 rounded"
            >
              {{ ip }}
            </button>
          </li>
        </ul>
        <div v-else-if="searchCompleted && !foundDevices.length" class="text-sm text-red-500 text-center">
          No se encontraron dispositivos.
        </div>

        <div v-if="connectionStatus" class="text-center text-sm text-gray-700">
          Estado: {{ connectionStatus }}
        </div>

        <!-- Botón Buscar en red -->
         <h1 class="flex justify-center mt-10 font-semibold">Búsqueda automática</h1>
        <button
          @click="scanNetwork"
          :disabled="searching"
          :class="[
            'w-full py-2 mt-2 font-semibold rounded-lg shadow-md',
            searching
              ? 'bg-gray-600 text-white cursor-not-allowed'
              : 'bg-blue-500 hover:bg-blue-600 text-white'
          ]">
          Buscar dispositivos en la red
        </button>
      </div>
    </div>

    <!-- Componente WebSocket (invisible) -->
    <ESP32WebSocket ref="webSocketComponent" />
    
  </div>
</template>

<script setup>
import JoystickComponent from './joystickComponent.vue'
import { onMounted, ref, onBeforeUnmount} from 'vue'
import ESP32WebSocket from '~/components/websocket.vue'

const flashOn = ref(false)
const modalOpen = ref(false)
const ipModalOpen = ref(false)
const cameraUrl = ref('') // URL de la cámara (puerto 82)
const manualIp = ref('')
const connectionStatus = ref('')
const searching = ref(false)
const searchCompleted = ref(false)
const foundDevices = ref([])
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
  const targetIp = ip || manualIp.value
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
    webSocketComponent.value.connectWebSocket(`ws://${targetIp}:81/`)
    
    // Escuchar eventos desde el componente WebSocket
    // Esto se manejará automáticamente por el componente
    
    connectionStatus.value = 'Conectado'
    ipModalOpen.value = false

  } catch (err) {
    connectionStatus.value = 'Error al conectar'
    console.error(err)
  }
}

const scanNetwork = async () => {
  searching.value = true
  searchCompleted.value = false
  foundDevices.value = []

  // Ajustar rango según red local
  const baseIp = '192.168.1.' //se asume que la red local es 192.168.1.x
  const promises = []

  for (let i = 1; i <= 254; i++) {
    const ip = baseIp + i
    promises.push(
      fetch(`http://${ip}:81/`, { mode: 'no-cors' })
        .then(() => foundDevices.value.push(ip) && console.log(`Dispositivo encontrado en ${ip}`))
      
        .catch(() => {}) // Silencia errores
    )
    
  }
  

  await Promise.all(promises)
  searching.value = false
  searchCompleted.value = true

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

<style scoped>
/* Puedes personalizar los estilos de los controles aquí si lo deseas */
</style>