<template>
  <div class="min-h-screen flex flex-col items-center bg-gray-100 p-4 space-y-4">
    <h1 class="text-2xl font-bold text-center">{{ currentCar.name }}</h1>

    <div class="flex flex-col-reverse md:flex-row w-full max-w-6xl gap-6 mt-4 items-center md:items-start">
      <!-- CONTROLES -->
      <div class="flex flex-col items-center md:items-start md:w-1/3 space-y-8">
        <!-- Botón Flash -->
        <button
          @click="toggleFlash"
          class="w-48 px-6 py-3 bg-yellow-400 hover:bg-yellow-500 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition-all duration-150"
        >
          {{ flashOn ? 'Apagar Flash' : 'Encender Flash' }}
        </button>

        <!-- Joystick -->
        <div class="w-20 h-58">
          <JoystickComponent />
        </div>

        <!-- Botón Cambiar Auto -->
        <button
          @click="modalOpen = true"
          class="w-48 px-6 py-3 mt-6 bg-green-500 hover:bg-green-600 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition-all duration-150"
        >
          Cambiar Auto
        </button>
      </div>

      <!-- CÁMARA -->
      <div class="w-full md:flex-1">
        <div class="aspect-video bg-black rounded-2xl overflow-hidden shadow-lg w-full">
          <img v-if="cameraUrl" :src="cameraUrl" alt="Cámara" class="w-full h-full object-cover" />
          <div v-else class="flex items-center justify-center h-full text-white">
            Esperando señal...
          </div>
        </div>
      </div>
    </div>

    <!-- MODAL Selección de Auto -->
    <div v-if="modalOpen" class="backdrop-blur-sm fixed inset-0 bg-black/60 bg-opacity-80 flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-2xl shadow-xl w-80 space-y-4">
        <h2 class="text-xl font-bold mb-4">Selecciona un Auto</h2>
        <ul class="space-y-2">
          <li v-for="car in availableCars" :key="car.id">
            <button @click="selectCar(car)" class="w-full text-left px-4 py-2 rounded-lg hover:bg-gray-200">
              {{ car.name }}
            </button>
          </li>
        </ul>
        <button @click="modalOpen = false" class="w-full mt-4 py-2 bg-red-400 hover:bg-red-500 text-white rounded-lg">
          Cerrar
        </button>
      </div>
    </div>

    <!-- BOTÓN Conectar -->
    <button
      @click="ipModalOpen = true"
      class="fixed bottom-6 right-6 px-6 py-3 bg-indigo-600 hover:bg-indigo-700 text-white font-semibold rounded-full shadow-lg transition-all duration-150 z-40"
    >
      Conectar ESP32
    </button>

    <!-- MODAL Conexión IP -->
    <div v-if="ipModalOpen" class="fixed inset-0 bg-black/60 backdrop-blur-sm flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-2xl shadow-xl w-96 space-y-4">
        <h2 class="text-xl font-bold text-center">Conectar al ESP32</h2>
        
        <label class="block">
          <span class="text-sm font-medium text-gray-700">Dirección IP del ESP32</span>
          <input
            v-model="manualIp"
            placeholder="ej: 192.168.1.123"
            class="mt-1 w-full px-4 py-2 border border-gray-300 rounded-lg shadow-sm focus:ring focus:ring-indigo-200 focus:outline-none"
          />
        </label>

        <!-- Resultados búsqueda -->
        <div v-if="searching" class="text-sm text-gray-500 text-center">
          Buscando dispositivos en red...
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

        <!-- Botones -->
        <div class="flex justify-end gap-4">
          <button
            @click="connectToESP"
            class="bg-indigo-500 hover:bg-indigo-600 text-white px-4 py-2 rounded-lg font-semibold shadow-md"
          >
            Conectar
          </button>
          <button
            @click="ipModalOpen = false"
            class="bg-gray-300 hover:bg-gray-400 text-gray-800 px-4 py-2 rounded-lg font-semibold shadow-md"
          >
            Cancelar
          </button>
        </div>

        <div v-if="connectionStatus" class="text-center text-sm text-gray-700">
          Estado: {{ connectionStatus }}
        </div>

        <!-- Botón Buscar en red -->
        <button
          @click="scanNetwork"
          class="w-full py-2 mt-2 bg-blue-500 hover:bg-blue-600 text-white rounded-lg font-semibold shadow-md"
        >
          Buscar en red
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount, watch } from 'vue'


const flashOn = ref(false)
const modalOpen = ref(false)
const ipModalOpen = ref(false)
const cameraUrl = ref('')
const manualIp = ref('')
const connectionStatus = ref('')
const searching = ref(false)
const searchCompleted = ref(false)
const foundDevices = ref([])
let socket = null

const currentCar = ref({ id: 1, name: 'Mi Auto ESP32' }) 
const availableCars = ref([]) //rellénalo desde API o backend

const toggleFlash = () => {
  flashOn.value = !flashOn.value
  sendCommand(flashOn.value ? 'FLASH_ON' : 'FLASH_OFF')
}

const selectCar = (car) => {
  currentCar.value = car
  modalOpen.value = false
}

const connectToESP = async (ip = null) => {
  const targetIp = ip || manualIp.value
  if (!targetIp) {
    connectionStatus.value = 'IP no válida'
    return
  }

  try {
    connectionStatus.value = 'Conectando...'
    socket?.close()

    socket = new WebSocket(`ws://${targetIp}:3000/ws`)

    socket.onopen = () => {
      connectionStatus.value = 'Conectado'
      cameraUrl.value = `http://${targetIp}:81/stream`
      ipModalOpen.value = false
    }

    socket.onmessage = (event) => {
      const msg = JSON.parse(event.data)
      // fluo de datos con imagen
      //if (msg.type === 'image') {
      //  console.log('Imagen recibida')
      //}
    }

    socket.onclose = () => {
      connectionStatus.value = 'Desconectado'
    }

    socket.onerror = () => {
      connectionStatus.value = 'Error de conexión'
    }

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
        .then(() => foundDevices.value.push(ip))
        .catch(() => {}) // Silencia errores
    )
  }

  await Promise.all(promises)
  searching.value = false
  searchCompleted.value = true
}

const sendCommand = (cmd) => {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ type: 'command', value: cmd }))
  } else {
    console.warn('WebSocket no conectado')
  }
}

onBeforeUnmount(() => {
  socket?.close()
})
</script>

<style scoped>
/* Puedes personalizar los estilos de los controles aquí si lo deseas */
</style>
