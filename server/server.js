const express = require('express');
const mqtt = require('mqtt');
const path = require('path');
const mysql = require('mysql2');

const app = express();
const port = 3000;

// Create a MySQL connection pool
const pool = mysql.createPool({
  host: 'localhost', // Or the IP address of your MySQL server
  user: 'root', // Your MySQL username
  password: 'lucho', // Your MySQL password
  database: 'sensor_data', // The name of your database
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
});

// Test the connection
pool.getConnection((err, connection) => {
  if (err) {
    console.error('Error connecting to database:', err);
    return;
  }
  console.log('Connected to the database.');
  connection.release(); // Release the connection
});

// Use the pool to execute queries
pool.query('SELECT 1 + 1 AS solution', (err, results) => {
  if (err) {
    console.error('Error executing query:', err);
    return;
  }
  console.log('The solution is:', results[0].solution);
});

// Close the pool when the application exits
process.on('exit', () => {
  pool.end();
});

// Connect to the MQTT broker
const client = mqtt.connect('ws://test.mosquitto.org:8080/mqtt');

let sensorData = {
  temperature: null,
  humidity: null,
  regar: null,
  alarmariego: null,
  humedadsuelo: null,
  current_tiempo: null
};

console.log('Server started');

function EventoConectar() {
  // Subscribe to the topics
  const topics = ['temperature', 'humidity', 'regar', 'alarmariego', 'humedadsuelo', 'current_tiempo'];
  topics.forEach(topic => {
    client.subscribe(topic, function(err) {
      if (!err) {
        console.log(`${topic} has been subscribed`);
      } else {
        console.error(`Failed to subscribe to ${topic}:`, err);
      }
    });
  });
}

function EventoMensaje(topic, message) {
  console.log(`Received message on topic ${topic}: ${message.toString()}`);
  sensorData[topic] = topic === 'current_tiempo' ? message.toString() : parseFloat(message.toString());

  // Insert the data into the database
  const query = `
    INSERT INTO sensors (temperature, humidity, regar, alarmariego, humedadsuelo, current_tiempo)
    VALUES (?, ?, ?, ?, ?, ?)
  `;
  const values = [
    sensorData.temperature,
    sensorData.humidity,
    sensorData.regar,
    sensorData.alarmariego,
    sensorData.humedadsuelo,
    sensorData.current_tiempo
  ];

  // Use the pool to get a connection and execute the query
  pool.query(query, values, (err, results) => {
    if (err) {
      console.error('Failed to insert data into the database:', err);
      return;
    }
    console.log('Sensor data inserted into the database:', results.insertId);
  });
}

client.on('connect', EventoConectar);
client.on('message', EventoMensaje);

// Handle errors
client.on('error', (error) => {
  console.error('MQTT Error:', error);
});

// Serve the static files in the public directory
app.use(express.static(path.join(__dirname, 'public')));

// API endpoint to fetch sensor data
app.get('/api/sensor-data', (req, res) => {
  res.json(sensorData);
});

app.listen(port, () => {
  console.log(`Server running at http://localhost:${port}`);
});

// Serve static files from the 'public' directory
app.use(express.static(path.join(__dirname, 'public')));

// Define a route for the root URL '/'
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// API endpoint to fetch data for charts
app.get('/datagraf', (req, res) => {
  const query = `
    SELECT temperature, humidity, humedadsuelo
    FROM sensors
    ORDER BY id DESC
    LIMIT 25
  `;

  pool.query(query, (err, results) => {
    if (err) {
      console.error('Failed to fetch data from the database:', err);
      res.status(500).json({ error: 'Internal Server Error' });
      return;
    }

    const data = {
      temperature: results.map(row => row.temperature),
      humidity: results.map(row => row.humidity),
      humidity_land: results.map(row => row.humedadsuelo)
    };

    res.json(data);
  });
});

