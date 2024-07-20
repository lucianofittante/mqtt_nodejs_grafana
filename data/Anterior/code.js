let temperatureChart, humidityChart, humidityLandChart, fullDataChart;

function updateCharts(data) {
    const labels = data.temperature.map((_, index) => index + 1);

    // Update temperature chart
    if (!temperatureChart) {
        const ctx = document.getElementById('temperatureChart').getContext('2d');
        temperatureChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Temperatura',
                    data: data.temperature,
                    borderColor: 'rgba(255, 99, 132, 1)',
                    backgroundColor: 'rgba(255, 99, 132, 0.2)',
                    fill: false,
                    borderWidth: 1
                }]
            },
            options: {
                scales: {
                    x: {
                        title: {
                            display: true,
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Values'
                        },
                        min: 0,
                        max: 60
                    }
                }
            }
        });
    } else {
        temperatureChart.data.labels = labels;
        temperatureChart.data.datasets[0].data = data.temperature;
        temperatureChart.update();
    }

    // Update humidity chart
    if (!humidityChart) {
        const ctx = document.getElementById('humidityChart').getContext('2d');
        humidityChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Humedad Amb',
                    data: data.humidity,
                    borderColor: 'rgba(54, 162, 235, 1)',
                    backgroundColor: 'rgba(54, 162, 235, 0.2)',
                    fill: false,
                    borderWidth: 1
                }]
            },
            options: {
                scales: {
                    x: {
                        title: {
                            display: true,
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Values'
                        },
                        min: 0,
                        max: 100
                    }
                }
            }
        });
    } else {
        humidityChart.data.labels = labels;
        humidityChart.data.datasets[0].data = data.humidity;
        humidityChart.update();
    }

    // Update humidity land chart
    if (!humidityLandChart) {
        const ctx = document.getElementById('humidityLandChart').getContext('2d');
        humidityLandChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Humedad Suelo',
                    data: data.humidity_land,
                    borderColor: 'rgba(75, 192, 192, 1)',
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    fill: false,
                    borderWidth: 1
                }]
            },
            options: {
                scales: {
                    x: {
                        title: {
                            display: true,
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Values'
                        },
                        min: 0,
                        max: 100
                    }
                }
            }
        });
    } else {
        humidityLandChart.data.labels = labels;
        humidityLandChart.data.datasets[0].data = data.humidity_land;
        humidityLandChart.update();
    }
}

function updateFullDataChart(data) {
    const labels = data.temperature.map((_, index) => index + 1);

    // Update full data chart
    if (!fullDataChart) {
        const ctx = document.getElementById('fullDataChart').getContext('2d');
        fullDataChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Temperature',
                    data: data.temperature,
                    borderColor: 'rgba(255, 99, 132, 1)',
                    backgroundColor: 'rgba(255, 99, 132, 0.2)',
                    fill: false,
                    borderWidth: 1
                }, {
                    label: 'Humidity',
                    data: data.humidity,
                    borderColor: 'rgba(54, 162, 235, 1)',
                    backgroundColor: 'rgba(54, 162, 235, 0.2)',
                    fill: false,
                    borderWidth: 1
                }, {
                    label: 'Soil Humidity',
                    data: data.humidity_land,
                    borderColor: 'rgba(75, 192, 192, 1)',
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    fill: false,
                    borderWidth: 1
                }]
            },
            options: {
                scales: {
                    x: {
                        title: {
                            display: true,
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Values'
                        },
                        min: 0,
                        max: 100
                    }
                }
            }
        });
    } else {
        fullDataChart.data.labels = labels;
        fullDataChart.data.datasets[0].data = data.temperature;
        fullDataChart.data.datasets[1].data = data.humidity;
        fullDataChart.data.datasets[2].data = data.humidity_land;
        fullDataChart.update();
    }
}

function fetchData() {
    fetch('/datagraf')
        .then(response => {
            if (response.ok) {
                return response.json();
            } else {
                throw new Error('Failed to fetch data');
            }
        })
        .then(data => {
            updateCharts(data);
        })
        .catch(error => {
            console.error('Error fetching data:', error);
        });
}

function fetchFullDataChart() {
    fetch('/datagraficas')
        .then(response => {
            if (response.ok) {
                return response.json();
            } else {
                throw new Error('Failed to fetch full data chart');
            }
        })
        .then(data => {
            updateFullDataChart(data);
        })
        .catch(error => {
            console.error('Error fetching full data chart:', error);
        });
}

function updateValues() {
  fetch('/data')
    .then(response => {
      if (response.ok) {
        return response.json();
      } else {
        throw new Error('Failed to fetch data');
      }
    })
    .then(data => {
      // Update the UI with received data
      document.getElementById('temperature').innerText = data.temperature;
      document.getElementById('humidity').innerText = data.humidity;
      document.getElementById('humedadsuelo').innerText = data.humedadsuelo;

      document.getElementById('regar').style.backgroundColor = data.regar ? 'green' : 'red';
      document.getElementById('alarmariego').style.backgroundColor = data.alarmariego ? 'green' : 'red';

      document.getElementById('current_time').innerText = data.time;
    })
    .catch(error => {
      console.error('Error fetching data:', error);
    });
}
function toggleRegar() {
    fetch('/regar')
        .then(response => {
            if (!response.ok) {
                throw new Error('Failed to toggle regar');
            }
        })
        .catch(error => {
            console.error('Error toggling regar:', error);
        });
}

function updateChartVisibility() {
    const selectedChart = document.getElementById('chartSelector').value;

    document.getElementById('temperatureChart').style.display = 'none';
    document.getElementById('humidityChart').style.display = 'none';
    document.getElementById('humidityLandChart').style.display = 'none';
    document.getElementById('fullDataChart').style.display = 'none';

    switch (selectedChart) {
        case 'temperature':
            document.getElementById('temperatureChart').style.display = 'block';
            break;
        case 'humidity':
            document.getElementById('humidityChart').style.display = 'block';
            break;
        case 'humidityLand':
            document.getElementById('humidityLandChart').style.display = 'block';
            break;
    }
}

document.addEventListener('DOMContentLoaded', function () {
    updateValues();
    fetchData();
    setInterval(updateValues, 1000);
    setInterval(fetchData, 1000);
    setInterval(fetchFullDataChart,1000);
});
