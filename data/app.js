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

function toggleRegar() {
  fetch('/regar', {
    method: 'GET'
  })
  .then(response => {
    if (response.ok) {
      // Toggle the button background color on success
      console.log('LED toggled successfully');
      return response.json();
    } else {
      throw new Error('Failed to toggle LED');
    }
  })
  .catch(error => {
    console.error('Error toggling LED:', error);
  });
}

window.onload = function() {
  console.log('Page loaded, updating values...');
  updateValues(); // Fetch initial values on load
  setInterval(updateValues, 1000); // Fetch values every second
};
