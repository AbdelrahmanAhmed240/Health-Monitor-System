// Time interval (in milliseconds) to fetch updated sensor data from the backend
const updateInterval = 1000;  // Fetch sensor data every second

// Function to retrieve sensor data from the backend API
async function fetchSensorData() {
  try {
    const response = await fetch('/api/sensors'); // API call to get latest sensor values
    if (!response.ok) throw new Error('Network response not ok');

    const data = await response.json();

    // Display the values on the web page; use fallback '--' if unavailable
    document.getElementById('temperature').textContent = data.temperature !== '--' ? data.temperature.toFixed(1) : '--';
    document.getElementById('bpm').textContent = data.BPM ?? '--';
    document.getElementById('spo2').textContent = data.SpO2 ?? '--';
    document.getElementById('bodyTemp').textContent = data.bodytemperature !== '--' ? data.bodytemperature.toFixed(1) : '--';
    document.getElementById('co').textContent = data.CO ?? '--';
    document.getElementById('co2').textContent = data.CO2 ?? '--';

  } catch (error) {
    console.error('Failed to fetch sensor data:', error);
  }
}

// Call once and then set interval for updates
fetchSensorData();
setInterval(fetchSensorData, updateInterval);

// Handle sending command from website to backend
const commandForm = document.getElementById('commandForm');
const commandStatus = document.getElementById('commandStatus');

commandForm.addEventListener('submit', async (e) => {
  e.preventDefault();

  const command = document.getElementById('commandSelect').value;
  if (!command) return;

  commandStatus.textContent = 'Sending command...';

  try {
    const response = await fetch('/commands', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ command }) // Send command to backend
    });

    if (!response.ok) throw new Error('Failed to send command');

    const result = await response.json();
    commandStatus.textContent = `Command sent successfully: ${result.message || command}`;
  } catch (error) {
    commandStatus.textContent = `Error sending command: ${error.message}`;
  }

  commandForm.reset(); // Clear form after submission
});
