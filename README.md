| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# MQTT Example

This example reads humidity in the ground and environment, temperature, soil moisture, and ground humidity warnings. It sends these values using MQTT to an online Mosquitto broker. All the data is received by server.js, which runs on a Raspberry Pi inside the mqtt-nodejs-server-raspberry folder. The data is saved in a MySQL database and displayed using Grafana.” 🌱📊


