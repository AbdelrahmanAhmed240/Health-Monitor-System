const express = require('express');
const mqtt = require('mqtt');
const cors = require('cors');

const app = express();
const port = 3000;

// Middleware setup
app.use(cors()); // Allow cross-origin requests
app.use(express.json()); // Parse JSON request bodies
app.use(express.static('public')); // Serve frontend files from 'public' folder

// MQTT Broker & Topics
const mqttBroker = 'mqtt://broker.hivemq.com';
const sensorTopic = 'yourhome/sensors';
const commandTopic = 'yourhome/commands';

// Object to store latest sensor values
let latestSensorData = {
  temperature: '--',
  BPM: '--',
  SpO2: '--',
  bodytemperature: '--',
  CO: '--',
  CO2: '--'
};

// Connect to MQTT broker
const client = mqtt.connect(mqttBroker);

client.on('connect', () => {
  console.log('Connected to MQTT broker');

  // Subscribe to topics
  client.subscribe(sensorTopic, (err) => {
    if (err) console.error('Subscription error on sensors:', err);
    else console.log(`Subscribed to: ${sensorTopic}`);
  });

  client.subscribe(commandTopic, (err) => {
    if (err) console.error('Subscription error on commands:', err);
    else console.log(`Subscribed to: ${commandTopic}`);
  });
});

// Handle incoming MQTT messages
client.on('message', (topic, message) => {
  if (topic === sensorTopic) {
    try {
      const payload = JSON.parse(message.toString());
      // Update sensor readings
      latestSensorData = {
        temperature: payload.temperature_room ?? '--',
        BPM: payload.BPM ?? '--',
        SpO2: payload.SpO2 ?? '--',
        bodytemperature: payload.temperature_body ?? '--',
        CO: payload.CO ?? '--',
        CO2: payload.CO2 ?? '--'
      };
      console.log('Updated sensor data:', latestSensorData);
    } catch (e) {
      console.error('Invalid JSON on sensor message:', e);
    }
  } else if (topic === commandTopic) {
    console.log('Received command message on MQTT:', message.toString());
  }
});

// Endpoint to get current sensor values
app.get('/api/sensors', (req, res) => {
  res.json(latestSensorData);
});

// Endpoint to send command to MQTT
app.post('/commands', (req, res) => {
  const { command } = req.body;
  if (!command) return res.status(400).json({ error: 'Command missing' });

  console.log('Publishing command:', command);

  client.publish(commandTopic, command, (err) => {
    if (err) {
      console.error('Failed to publish command:', err);
      return res.status(500).json({ error: 'Failed to send command' });
    }
    res.json({ message: `Command '${command}' sent` });
  });
});

// Start server
app.listen(port, () => {
  console.log(`Backend server listening at http://localhost:${port}`);
});
