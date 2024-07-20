| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# MQTT Example

This example reads humidity in the ground and environment, temperature, regar state, and warning humidity ground. It sends these values using MQTT and an online Mosquitto broker. All the data is received by server.js, which is running on another computer. The data is saved in a MySQL database.


