'use strict';

// window.chartColors = {
//     red: 'rgb(255, 99, 132)',
//     orange: 'rgb(255, 159, 64)',
//     yellow: 'rgb(255, 205, 86)',
//     green: 'rgb(75, 192, 192)',
//     blue: 'rgb(54, 162, 235)',
//     purple: 'rgb(153, 102, 255)',
//     grey: 'rgb(201, 203, 207)'
// };

// var client = mqtt.connect('mqtt://ufo.dynatrace.cloud:8883', {protocolId: "MQTT", username: 'dtufo', password: 'dtufo123'});
var client = mqtt.connect('wss://test.mosquitto.org:8081');

setTimeout(() => {
    client.publish("TopicToSubscribe", "Hello world!");
}, 5000);

client.on("message", (topic, message, packet) => {
    // Log message
    console.log(topic);
    console.log(message.toString());
    console.log(packet);
    // Close the connection
    client.end();
});

client.on("connect", () => {
    client.subscribe("TopicToSubscribe");
    console.log("Connected to MQTT Broker.");
});

var jsonData = '{"timestamp":"1827609","device":{"id":"ufo-30aea420be35","clientIP":"172.28.165.246","ssid":"Royal-Guest","version":"Dec 17 2018 - 12:02:57","build":"1000","cpu":"ESP32","battery":"100","temperature":"22.12","leds": {"logo": ["ff0000", "00ff40", "00ff00", "000044"], "top": {"color": ["ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ff0000","ffff00","ff0000"],"background": "000000","whirl": {"speed": 0,"clockwise": 0},"morph": { "state": 1, "period": 80, "periodTick": 80, "speed": 8, "speedTick": 1, "percentage": 68}},"bottom": {"color": ["004400","ff4400","ff4400","ff4400","004400","004400","004400","004400","004400","004400","004400","004400","004400","004400","004400"],"background": "000000","whirl": {"speed": 0,"clockwise": 0},"morph": { "state": 1, "period": 80, "periodTick": 80, "speed": 8, "speedTick": 3, "percentage": 70}}},"freemem":"143588"}}';
var json = JSON.parse(jsonData);

var logoColors = json.device.leds.logo;
var topColors = json.device.leds.top.color;
var bottomColors = json.device.leds.bottom.color;
var noOfLeds = topColors.length;

console.log(logoColors);
document.getElementById('logo1').style.backgroundColor = '#' + logoColors[0];
document.getElementById('logo2').style.backgroundColor = '#' + logoColors[1];
document.getElementById('logo3').style.backgroundColor = '#' + logoColors[2];
document.getElementById('logo4').style.backgroundColor = '#' + logoColors[3];

var createLEDring = function () {
    var arr = new Array(noOfLeds);
    for (var i = 0; i < noOfLeds; i++) {
        arr[i] = 1;
    }
    return arr;
};

var createLabels = function () {
    var arr = new Array(noOfLeds);
    for (var i = 0; i < noOfLeds; i++) {
        arr[i] = 'LED #' + (i + 1);
    }
    return arr;
}

var fillTopColors = function () {
    var colorArray = new Array(noOfLeds);
    for (var i = 0; i < noOfLeds; i++) {
        colorArray[i] = '#' + topColors[i];
    }
    return colorArray;
}

var fillBottomColors = function () {
    var colorArray = new Array(noOfLeds);
    for (var i = 0; i < noOfLeds; i++) {
        colorArray[i] = '#' + bottomColors[i];
    }
    return colorArray;
}

var whirl = function (clockwise) {
    if (clockwise) {
        config.data.datasets[0].backgroundColor.unshift(config.data.datasets[0].backgroundColor.pop());
    } else {
        config.data.datasets[0].backgroundColor.push(config.data.datasets[0].backgroundColor.shift());
    }
    window.myPie.update();
}

var config = {
    type: 'doughnut',
    data: {
        datasets: [{
            data: createLEDring(),
            backgroundColor: fillBottomColors(),
            label: 'Bottom Ring'
        }, {
            data: createLEDring(),
            backgroundColor: fillTopColors(),
            label: 'Top Ring'
        }],
        labels: createLabels()
    },
    options: {
        responsive: true,
        legend: false,
        multiTooltipTemplate: '<%= datasetLabel %> - <%= value %>'
    }
};

window.onload = function () {
    var ctx = document.getElementById('chart-area').getContext('2d');
    window.myPie = new Chart(ctx, config);
};

document.getElementById('updateStatus').addEventListener('click', function () {
    config.data.datasets.forEach(function (dataset) {
        dataset.backgroundColor = fillTopColors();
    });

    window.myPie.update();
});

document.getElementById("whirlcw").addEventListener('click', function () {
    whirl(true);
})

document.getElementById("whirlccw").addEventListener('click', function () {
    whirl(false);
})

document.getElementById("startwhirl").addEventListener('click', function () {
    window.setInterval(function () {
        whirl(false);
    }, 500);
})

// var colorNames = Object.keys(window.chartColors);
// document.getElementById('addDataset').addEventListener('click', function() {
//     var newDataset = {
//         backgroundColor: [],
//         data: [],
//         label: 'New dataset ' + config.data.datasets.length,
//     };

//     for (var index = 0; index < config.data.labels.length; ++index) {
//         newDataset.data.push(randomScalingFactor());

//         var colorName = colorNames[index % colorNames.length];
//         var newColor = window.chartColors[colorName];
//         newDataset.backgroundColor.push(newColor);
//     }

//     config.data.datasets.push(newDataset);
//     window.myPie.update();
// });

// document.getElementById('removeDataset').addEventListener('click', function() {
//     config.data.datasets.splice(0, 1);
//     window.myPie.update();
// });