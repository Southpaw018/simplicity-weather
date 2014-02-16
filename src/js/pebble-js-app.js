//Partially adapted (and mostly stolen) from github.com/Niknam/futura-weather-sdk2.0 / src / js / pebble-js-app.js

Pebble.addEventListener("ready", function(e) {
	console.log("Starting ...");
	updateWeather();
});

Pebble.addEventListener("appmessage", function(e) {
	console.log("Got a message - Starting weather request...");
	updateWeather();
});

var updateInProgress = false;

function updateWeather() {
	if (!updateInProgress) {
		updateInProgress = true;
		var locationOptions = { "timeout": 15000, "maximumAge": 60000 };
		navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
	}
	else {
		console.log("Not starting a new request. Another one is in progress...");
	}
}

function locationSuccess(pos) {
	var coordinates = pos.coords;
	console.log("Got coordinates: " + JSON.stringify(coordinates));
	fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
	console.warn('Location error (' + err.code + '): ' + err.message);
	Pebble.sendAppMessage({ "error": "Loc unavailable" });
	updateInProgress = false;
}

function fetchWeather(latitude, longitude) {
	var response;
	var req = new XMLHttpRequest();
	req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				console.log(req.responseText);
				response = JSON.parse(req.responseText);
				var temperature;
				if (response) {
					var tempResult = response.main.temp;
					if (response.sys.country === "US") {
						// Convert temperature to Fahrenheit if user is within the US
						temperature = Math.round(((tempResult - 273.15) * 1.8) + 32) + "\u00B0F";
					}
					else {
						// Otherwise, convert temperature to Celsius
						temperature = Math.round(tempResult - 273.15) + "\u00B0C";
					}		 
					//var condition = response.weather[0].id;
					var city = response.name;

					console.log("Temperature: " + temperature + " City: " + city);

					Pebble.sendAppMessage({
						"temperature": temperature,
						"city": city
					});
					updateInProgress = false;
				}
			} else {
				console.log("Error");
				updateInProgress = false;
				Pebble.sendAppMessage({ "error": "HTTP Error" });
			}
		}
	};
	req.send(null);
}