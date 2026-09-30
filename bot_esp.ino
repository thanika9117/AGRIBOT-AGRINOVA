#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

/* WIFI */

const char* ssid = "riju";
const char* password = "riju2007";

AsyncWebServer server(80);

/* DHT SENSOR */

#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

/* GPS */

TinyGPSPlus gps;
SoftwareSerial gpsSerial(D3, D4);

/* VARIABLES */

float temperature = 0;
float humidity = 0;

String rain = "Dry";
String disease = "Negative";
int severity = 0;

String gpsLink = "#";

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 3000;

const int SERIAL_BUFFER_MAX = 100;
char serialBuffer[100];
int serialIndex = 0;

/* ================= DASHBOARD TEMPLATE (UNCHANGED) ================= */

const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html lang="en">

<head>

<meta charset="UTF-8">

<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>AGRIBOT - AGRINOVA</title>

<style>

:root {

--bg-color:#eaf5e8;
--card-bg:#ffffff;
--text-main:#1f331b;
--text-muted:#4a6b43;
--accent:#a3d9a5;
--accent-hover:#81c784;
--border-color:#cce5cc;

}

body {

font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;
background-color:var(--bg-color);
color:var(--text-main);
margin:0;
padding:20px;
box-sizing:border-box;

}

.container {

max-width:1200px;
margin:0 auto;

}

.header {

text-align:center;
margin-bottom:30px;
padding:15px;
background:var(--card-bg);
border-radius:12px;
box-shadow:0 4px 6px rgba(0,0,0,0.05);

}

.header h1 {

margin:0;
font-size:2rem;
letter-spacing:1px;

}

.dashboard-grid {

display:grid;
grid-template-columns:2fr 1fr;
gap:20px;
margin-bottom:20px;

}

@media (max-width:900px) {

.dashboard-grid {

grid-template-columns:1fr;

}

}

.card {

background:var(--card-bg);
padding:20px;
border-radius:12px;
box-shadow:0 4px 6px rgba(0,0,0,0.05);

}

.card-title {

margin-top:0;
margin-bottom:15px;
font-size:1.25rem;
border-bottom:2px solid var(--border-color);
padding-bottom:10px;

}

.video-placeholder {

width:100%;
aspect-ratio:16/9;
background-color:#2c3e2e;
border-radius:8px;
display:flex;
align-items:center;
justify-content:center;
color:#a3d9a5;
font-size:1.2rem;
font-weight:bold;
text-transform:uppercase;
letter-spacing:2px;

}

.stats-grid {

display:grid;
grid-template-columns:1fr 1fr;
gap:15px;

}

.stat-item {

background:var(--bg-color);
padding:15px;
border-radius:8px;
border:1px solid var(--border-color);
text-align:center;

}

.stat-label {

font-size:0.85rem;
color:var(--text-muted);
text-transform:uppercase;
font-weight:bold;
margin-bottom:8px;

}

.stat-value {

font-size:1.4rem;
font-weight:bold;

}

.stat-value a {

color:#2e7d32;
text-decoration:none;

}

.stat-value a:hover {

text-decoration:underline;

}

</style>

</head>

<body>

<div class="container">

<header class="header">

<h1>AGRIBOT - AGRINOVA</h1>

</header>

<div class="dashboard-grid">

<div class="card">

<h2 class="card-title">Live Stream</h2>

<div class="video-placeholder">

[ VIDEO FEED OFFLINE ]

</div>

</div>

<div class="card">

<h2 class="card-title">Sensor Data</h2>

<div class="stats-grid">

<div class="stat-item">

<div class="stat-label">Temperature</div>

<div class="stat-value" id="val-temp">-- °C</div>

</div>

<div class="stat-item">

<div class="stat-label">Humidity</div>

<div class="stat-value" id="val-humid">-- %</div>

</div>

<div class="stat-item">

<div class="stat-label">Precipitation</div>

<div class="stat-value" id="val-rain">Dry</div>

</div>

<div class="stat-item">

<div class="stat-label">GPS Location</div>

<div class="stat-value"><a id="val-gps" href="#">Maps Link</a></div>

</div>

<div class="stat-item">

<div class="stat-label">Disease Detect</div>

<div class="stat-value" id="val-disease">Negative</div>

</div>

<div class="stat-item">

<div class="stat-label">Severity</div>

<div class="stat-value" id="val-severity">0%</div>

</div>

</div>

</div>

</div>

</div>

<script>

function updateData(){

fetch("/sensor")

.then(res => res.json())

.then(data => {

document.getElementById("val-temp").innerText = data.temperature + " °C";
document.getElementById("val-humid").innerText = data.humidity + " %";
document.getElementById("val-rain").innerText = data.rain;
document.getElementById("val-disease").innerText = data.disease;
document.getElementById("val-severity").innerText = data.severity + "%";
document.getElementById("val-gps").href = data.gps;

});

}

setInterval(updateData,1000);

</script>

</body>

</html>

)rawliteral";

/* ================= READ RAIN FROM ARDUINO ================= */

void readArduino(){

while (Serial.available()){

char c = Serial.read();

if (c == '\n'){

serialBuffer[serialIndex] = '\0';

rain = String(serialBuffer);
rain.trim();

serialIndex = 0;

}

else{

if (serialIndex < SERIAL_BUFFER_MAX - 1){

serialBuffer[serialIndex] = c;
serialIndex++;

}
else{

serialIndex = 0;

}

}

}

}

/* ================= SETUP ================= */

void setup(){

Serial.begin(9600);
gpsSerial.begin(9600);

dht.begin();

WiFi.begin(ssid,password);

while(WiFi.status()!=WL_CONNECTED){
delay(500);
}

Serial.println();
Serial.print("Dashboard: http://");
Serial.println(WiFi.localIP());

server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
request->send_P(200,"text/html",index_html);
});

server.on("/sensor",HTTP_GET,[](AsyncWebServerRequest *request){

JsonDocument doc;

doc["temperature"]=temperature;
doc["humidity"]=humidity;
doc["rain"]=rain;
doc["gps"]=gpsLink;
doc["disease"]=disease;
doc["severity"]=severity;

String json;
serializeJson(doc,json);

request->send(200,"application/json",json);

});

/* ================= RECEIVE DISEASE ================= */

server.on("/receive",HTTP_POST,[](AsyncWebServerRequest *request){
AsyncWebServerResponse *r = request->beginResponse(200,"application/json","{\"ok\":true}");
r->addHeader("Access-Control-Allow-Origin","*");
request->send(r);
},NULL,[](AsyncWebServerRequest *request,uint8_t *data,size_t len,size_t index,size_t total){

JsonDocument doc;
deserializeJson(doc,data,len);

disease = doc["label"].as<String>();
severity = (int)(doc["confidence"].as<float>()*100);

/* SEND COMMAND ONLY FOR DISEASE */

if(
disease == "YellowLeaf Curl Virus" ||
disease == "mosaic virus" ||
disease == "spectoria leaf spot"
){

if(gps.location.isValid()){
gpsLink="https://maps.google.com/?q="+String(gps.location.lat(),6)+","+String(gps.location.lng(),6);
}

Serial.println("STOPR");

}

});

server.begin();

}

/* ================= LOOP ================= */

void loop(){

readArduino();

while(gpsSerial.available()){
gps.encode(gpsSerial.read());
}

unsigned long now=millis();

if(now-lastSensorRead>=SENSOR_INTERVAL){

lastSensorRead=now;

float t=dht.readTemperature();
float h=dht.readHumidity();

if(!isnan(t)) temperature=t;
if(!isnan(h)) humidity=h;

}

}