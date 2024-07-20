Chart.defaults.color = '#fff';
Chart.defaults.borderColor = '#444';

// Function to update the values displayed on the page
const updateValues = () => {
    fetch('/data')
        .then(response => {
            if (response.ok) {
                return response.json();
            } else {
                throw new Error('Failed to fetch data');
            }
        })
        .then(data => {
            // Update UI with sensor data
            document.getElementById('temperature').innerText = data.temperature;
            document.getElementById('humidity').innerText = data.humidity;
            document.getElementById('humedadsuelo').innerText = data.humedadsuelo;
            
            // Set background color based on regar and alarmariego values
            document.getElementById('regar').style.backgroundColor = data.regar ? 'green' : 'red';
            document.getElementById('alarmariego').style.backgroundColor = data.alarmariego ? 'green' : 'red';
            
            // Display current time
            document.getElementById('current_time').innerText = data.time;

            // Debugging output to check data.logs content
            console.log('Received logs:', data.logs);

            // Display MQTT logs
            document.getElementById('logs').innerHTML = data.logs; // Use innerHTML to render HTML content
        })
        .catch(error => {
            console.error('Error fetching data:', error);
        });
};

// Function to toggle the 'regar' state
const toggleRegar = () => {
    fetch('/regar')
        .then(response => {
            if (!response.ok) {
                throw new Error('Failed to toggle regar');
            }
        })
        .catch(error => {
            console.error('Error toggling regar:', error);
        });
};

// Initialize the page
document.addEventListener('DOMContentLoaded', function () {
    updateValues();
    setInterval(updateValues, 1000);
});
