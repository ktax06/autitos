<script setup>
import { io } from "socket.io-client";
import { onMounted } from "vue";

onMounted(() => {
  const socket = io("http://localhost:3000");

  socket.on("connect", () => {
    console.log("Conectado al servidor de WebSocket");
  });

  socket.on("to-client", (data) => {
    console.log("Mensaje desde ESP32:", data);
  });
});
</script>
