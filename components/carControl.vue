<template>
  <div class="min-h-screen flex flex-col items-center bg-gray-100 p-4 space-y-4">
 
    <!-- Nombre del Auto -->
    <h1 class="text-2xl font-bold text-center">{{ currentCar.name }}</h1>

    <!-- Contenido principal -->
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
          <JoystickComponent/>
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

    <!-- MODAL de selección de Auto -->
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

  </div>
</template>

<script setup>
import { ref } from 'vue'
import JoystickComponent from './joystickComponent.vue'

const flashOn = ref(false)
const modalOpen = ref(false)
const cameraUrl = ref('') // Aquí puedes colocar la URL de la cámara en vivo.
const currentCar = ref({ name: 'Mi Auto' })
const availableCars = ref([
  { id: 1, name: 'Auto 1' },
  { id: 2, name: 'Auto 2' },
])

function toggleFlash() {
  flashOn.value = !flashOn.value
}

function move(direction) {
  console.log('Mover dirección:', direction)
  // Aquí enviarías el comando a tu WebSocket o lo que uses para controlar el auto
}

function selectCar(car) {
  currentCar.value = car
  modalOpen.value = false
}

// Función para manejar el movimiento del joystick
function onJoystickMove(evt) {
  const dir = evt.direction // Puede ser up, down, left, right
  move(dir)
}

function onJoystickEnd() {
  console.log('Joystic stopped')
}
</script>

<style scoped>
/* Puedes personalizar los estilos de los controles aquí si lo deseas */
</style>
