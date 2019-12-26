#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "DHT.h"
#include <EEPROM.h>
#include <ESP8266WebServer.h>
//#include <ThingSpeak.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
 
#define ONE_WIRE_PIN D2
#define RELAYPIN D1
#define DHTPIN D3     // what pin we're connected to
#define DHTTYPE DHT22
#define buttonUpPin D4
#define buttonDownPin A0

//Configuration
const char* ssid = "safe_local";
const char* password = "ew6NuaOIdLb3CKmCKS";
const long  pumpInterval = 10000; //Check  @ 10seconds
const long  loggingInterval = 60000; //Check  @ 60seconds
const long  secPumpAfterTempReach = 60000; //La pompe continue 60 secondes après que la plancher à atteind la consigne
const float const_overShoot = 1.0;

float setTemperature = 15.0;  //15.0 C par défaut
float tempThreshold = 0.20; //combien de degré +/- avant de déclancher la pompe
float overShoot = 0.0;
bool  indModePointe = false;

//Variables
unsigned long previousMillis = 0;
unsigned long previousPumpIconMillis = 0;
boolean pumpState = false;

int buttonUpState = 0;         // variable for reading the pushbutton status
int buttonDownState = 0;         // variable for reading the pushbutton status
int lastButtonUpState = 0;         // variable for reading the pushbutton status
int lastButtonDownState = 1;         // variable for reading the pushbutton status

float floorTemp = 0;
float roomTemp = 0;
float roomHum = 0;
float inTemp = 0;
float outTemp = 0;
bool pausePointe = false;
int lastPumpIcon = 1;


// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
int EEaddress = 0;

// ThingSpeak Settings
const int channelID = 745467;
const char* writeAPIKey = "3B2SA0H09K6CEQH7"; // write API key for your ThingSpeak Channel
const char* server = "api.thingspeak.com";

//WiFiServer server(80); // Set web server port number to 80
ESP8266WebServer serverWeb ( 80 );
WiFiClient client;
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h 

WiFiUDP ntpUDP;
// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP, "ca.pool.ntp.org", -18000, 3600000);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors_floor(&oneWire);
//DeviceAddress adrFloorSensor = { 0x28, 0xEE, 0x10, 0xB8, 0x1D, 0x16, 0x01, 0x97 };
DeviceAddress adrFloorSensor = { 0x28, 0xFF, 0x85, 0x3F, 0xC1, 0x16, 0x04, 0x5C };
DeviceAddress adrInSensor = { 0x28, 0xFF, 0x9A, 0x56, 0xB5, 0x16, 0x03, 0xBE };
DeviceAddress adrOutSensor = { 0x28, 0xEE, 0x0A, 0xCE, 0x1A, 0x16, 0x02, 0x6F };


DHT dht(DHTPIN, DHTTYPE, 11); // Initialize DHT sensor for normal 16mhz Arduino

// Image de pompe
const unsigned char pumpIcon_1 [] PROGMEM = 
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x0b, 0xf4, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0xe0, 0x00, 0x00, 
0x00, 0x07, 0xfd, 0x2f, 0xf8, 0x00, 0x00, 
0x00, 0x0f, 0xc0, 0x01, 0xfc, 0x00, 0x00, 
0x00, 0x1f, 0x00, 0x00, 0x3e, 0x00, 0x00, 
0x00, 0x7e, 0x00, 0x00, 0x1f, 0x80, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x07, 0x80, 0x00, 
0x00, 0xf0, 0x00, 0x00, 0x03, 0xc0, 0x00, 
0x01, 0xe0, 0x01, 0x80, 0x01, 0xe0, 0x00, 
0x03, 0xe0, 0x03, 0x80, 0x00, 0xf0, 0x00, 
0x03, 0xc0, 0x07, 0x00, 0x00, 0xf0, 0x00, 
0x07, 0x80, 0x07, 0x80, 0x00, 0x78, 0x00, 
0x07, 0x00, 0x07, 0x80, 0x00, 0x38, 0x00, 
0x0f, 0x00, 0x0f, 0x80, 0x00, 0x3c, 0x00, 
0x0e, 0x00, 0x0f, 0x00, 0x00, 0x3c, 0x00, 
0x0e, 0x00, 0x07, 0x80, 0x00, 0x1c, 0x00, 
0x1e, 0x00, 0x07, 0x80, 0x80, 0x1c, 0x00, 
0x0e, 0x00, 0x07, 0x8f, 0xe0, 0x1e, 0x00, 
0x1c, 0x00, 0x07, 0x1f, 0xf8, 0x0e, 0x00, 
0x1e, 0x00, 0x01, 0xbf, 0xf8, 0x1e, 0x00, 
0x1c, 0x00, 0x01, 0xff, 0xfc, 0x0e, 0x00, 
0x1c, 0x0d, 0xb6, 0xc0, 0x00, 0x0e, 0x00, 
0x1e, 0x0f, 0xff, 0x60, 0x00, 0x1e, 0x00, 
0x1c, 0x07, 0xfe, 0x70, 0x00, 0x0e, 0x00, 
0x0e, 0x03, 0xfc, 0x78, 0x00, 0x1c, 0x00, 
0x1e, 0x00, 0xb0, 0x78, 0x00, 0x1e, 0x00, 
0x0e, 0x00, 0x00, 0x78, 0x00, 0x1c, 0x00, 
0x0f, 0x00, 0x00, 0x7c, 0x00, 0x3c, 0x00, 
0x07, 0x00, 0x00, 0x78, 0x00, 0x38, 0x00, 
0x07, 0x00, 0x00, 0x78, 0x00, 0x38, 0x00, 
0x07, 0x80, 0x00, 0x78, 0x00, 0x78, 0x00, 
0x03, 0xc0, 0x00, 0x70, 0x00, 0xf0, 0x00, 
0x03, 0xc0, 0x00, 0x70, 0x00, 0xf0, 0x00, 
0x01, 0xe0, 0x00, 0x40, 0x01, 0xe0, 0x00, 
0x00, 0xf0, 0x00, 0x00, 0x03, 0xc0, 0x00, 
0x00, 0x7c, 0x00, 0x00, 0x07, 0x80, 0x00, 
0x00, 0x7c, 0x00, 0x00, 0x1f, 0x80, 0x00, 
0x00, 0x1f, 0x80, 0x00, 0x7e, 0x00, 0x00, 
0x00, 0x0f, 0xc0, 0x01, 0xfc, 0x00, 0x00, 
0x00, 0x07, 0xfd, 0x2f, 0xf0, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0xc0, 0x00, 0x00, 
0x00, 0x00, 0x7f, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x07, 0xf4, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Image de pompe
const unsigned char pumpIcon_2 [] PROGMEM = 
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x0b, 0xf8, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0xe0, 0x00, 0x00, 
0x00, 0x07, 0xfa, 0x2b, 0xf8, 0x00, 0x00, 
0x00, 0x1f, 0xc0, 0x02, 0xfc, 0x00, 0x00, 
0x00, 0x1e, 0x80, 0x00, 0x3f, 0x00, 0x00, 
0x00, 0x7c, 0x00, 0x00, 0x17, 0x00, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x0f, 0xc0, 0x00, 
0x01, 0xf0, 0x00, 0x00, 0x03, 0xc0, 0x00, 
0x01, 0xe0, 0x00, 0x00, 0x01, 0xe0, 0x00, 
0x03, 0xa0, 0x00, 0x00, 0x01, 0x60, 0x00, 
0x03, 0xc0, 0x00, 0x01, 0x00, 0xf0, 0x00, 
0x07, 0x80, 0x00, 0x0f, 0xc0, 0x78, 0x00, 
0x07, 0x01, 0x00, 0x3f, 0x00, 0x38, 0x00, 
0x0f, 0x01, 0x80, 0x2f, 0x00, 0x7c, 0x00, 
0x0e, 0x03, 0xc0, 0x7e, 0x00, 0x18, 0x00, 
0x0e, 0x01, 0xe0, 0x7c, 0x00, 0x1e, 0x00, 
0x1e, 0x01, 0xf0, 0xf8, 0x00, 0x1c, 0x00, 
0x0c, 0x01, 0xf8, 0xf0, 0x00, 0x1e, 0x00, 
0x1e, 0x00, 0xfc, 0xe0, 0x00, 0x1c, 0x00, 
0x1c, 0x00, 0x7e, 0xc0, 0x00, 0x0e, 0x00, 
0x1e, 0x00, 0x3f, 0x54, 0x00, 0x1e, 0x00, 
0x1c, 0x00, 0x05, 0xfe, 0x00, 0x0e, 0x00, 
0x1c, 0x00, 0x01, 0x5f, 0x80, 0x1c, 0x00, 
0x1e, 0x00, 0x03, 0xdf, 0xc0, 0x1e, 0x00, 
0x0c, 0x00, 0x07, 0x87, 0xc0, 0x1c, 0x00, 
0x1e, 0x00, 0x0f, 0x87, 0xe0, 0x1c, 0x00, 
0x0e, 0x00, 0x0f, 0x81, 0xe0, 0x1c, 0x00, 
0x0f, 0x00, 0x3f, 0x01, 0xc0, 0x3c, 0x00, 
0x0f, 0x00, 0x3e, 0x00, 0x60, 0x38, 0x00, 
0x07, 0x00, 0xfc, 0x00, 0x60, 0x78, 0x00, 
0x07, 0x80, 0xf0, 0x00, 0x00, 0x78, 0x00, 
0x03, 0xc0, 0x00, 0x00, 0x00, 0xe0, 0x00, 
0x01, 0xc0, 0x00, 0x00, 0x01, 0xf0, 0x00, 
0x02, 0xf0, 0x00, 0x00, 0x01, 0xc0, 0x00, 
0x01, 0xf0, 0x00, 0x00, 0x07, 0xc0, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x07, 0x80, 0x00, 
0x00, 0x7e, 0x00, 0x00, 0x1f, 0x00, 0x00, 
0x00, 0x1f, 0x40, 0x00, 0x7e, 0x00, 0x00, 
0x00, 0x0f, 0xd0, 0x03, 0xfa, 0x00, 0x00, 
0x00, 0x03, 0xfe, 0xbf, 0xf0, 0x00, 0x00, 
0x00, 0x02, 0xff, 0xff, 0xa0, 0x00, 0x00, 
0x00, 0x00, 0x5f, 0xfd, 0x40, 0x00, 0x00, 
0x00, 0x00, 0x05, 0xd4, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

String getPage(){
  sensors_floor.requestTemperatures(); //Obtenir la temperature  de tous les sensors
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature() - 1;

  float h = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    
  }
  
  String page = "<html lang='fr'><head><meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/>";
  page += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
  page += "<title>Plancher chauffant - Garage</title></head><body>";
  page += "<div class='container-fluid'>";
  page +=   "<div class='row'>";
  page +=     "<div class='col-md-12'>";
  page +=       "<h1>Plancher chauffant - Garage</h1>";
  page +=       "<h3>Thermostat</h3>";
  page +=       "<ul class='nav nav-pills'>";
  page +=         "<li class='active'>";
  page +=           "<a href='#'> <span class='badge pull-right'>";
  page +=           sensors_floor.getTempC(adrFloorSensor);
  page +=           "</span> Temp&eacute;rature</a>";
  page +=         "</li><li>";
  page +=           "<a href='#'> <span class='badge pull-right'>";
  page +=           h;
  page +=           "</span> Humidit&eacute;</a>";
  page +=         "</li><li>";
  page +=           "<a href='#'> <span class='badge pull-right'>";
  //page +=           p;
  page +=           "</span> Pression atmosph&eacute;rique</a></li>";
  page +=       "</ul>";
  page +=       "<table class='table'>";  // Tableau des relevés
  page +=         "<thead><tr><th>Capteur</th><th>Mesure</th><th>Valeur</th></tr></thead>"; //Entête
  page +=         "<tbody>";  // Contenu du tableau
  page +=           "<tr><td>Ambiant</td><td>Temp&eacute;rature</td><td>"; // Première ligne : température
  page +=             t;
  page +=             "&deg;C</td><td>";
  page +=           "<tr class='active'><td>Ambiant</td><td>Humidit&eacute;</td><td>"; // 2nd ligne : Humidité
  page +=             h;
  page +=             "%</td><td>";
  page +=           "<tr class='active'><td>Plancher</td><td>Temp&eacute;rature</td><td>"; // 3ème ligne : PA (BMP180)
  page +=             sensors_floor.getTempC(adrFloorSensor);
  page +=             "&deg;C</td><td>";
  page +=           "<tr class='active'><td>Pompe</td><td>Active</td><td>"; // 3ème ligne : PA (BMP180)
  page +=             pumpState;
  page +=           "</td><td>";
  page +=       "</tbody></table>";
  page +=       "<h3>Consigne</h3>";
  page +=       "<div class='row'>";
  page +=         "<div class='col-md-4'><h4 class ='text-left'>";
  page +=             setTemperature;
  page +=           "<span class='badge'>";
  page +=         "</span></h4></div>";
  page +=         "<div class='col-md-4'><form action='/consigne_down' method='POST'><button type='button submit' name='CONSIGNE_DOWN' value='0.5' class='btn btn-danger btn-lg'>DOWN</button></form></div>";
  page +=         "<div class='col-md-4'><form action='/consigne_up' method='POST'><button type='button submit' name='CONSIGNE_UP' value='0.5' class='btn btn-success btn-lg'>UP</button></form></div>";
  page +=       "</div>";
  page +=       "<h3>Config - Threshold</h3>";
  page +=       "<div class='row'>";
  page +=         "<div class='col-md-4'><h4 class ='text-left'>";
  page +=             tempThreshold;
  page +=           "<span class='badge'>";
  page +=         "</span></h4></div>";
  page +=         "<div class='col-md-4'><form action='/threshold_down' method='POST'><button type='button submit' name='THRESHOLD_DOWN' value='0.05' class='btn btn-danger btn-lg'>DOWN</button></form></div>";
  page +=         "<div class='col-md-4'><form action='/threshold_up' method='POST'><button type='button submit' name='THRESHOLD_UP' value='0.05' class='btn btn-success btn-lg'>UP</button></form></div>";    
  page +=       "</div>";
page +=       "<h3>Pointe hivernale</h3>";
  page +=       "<div class='row'>";
  page +=         "<div class='col-md-4'><h4 class ='text-left'>";
  page +=             indModePointe;
  page +=           "<span class='badge'>";
  page +=         "</span></h4></div>";
  page +=         "<div class='col-md-4'><form action='/pointe_off' method='POST'><button type='button submit' name='POINTE_OFF' value='false' class='btn btn-danger btn-lg'>OFF</button></form></div>";
  page +=         "<div class='col-md-4'><form action='/pointe_on' method='POST'><button type='button submit' name='POINTE_ON' value='true' class='btn btn-success btn-lg'>ON</button></form></div>";    
  page +=       "</div>";  
  page +=     "<br>";
  page += "</div></div></div>";
  page += "</body></html>";
  
  return page;
}

void handleRoot(){ 
 /* if ( server.hasArg("CONSIGNE_UP") ) {
    setTemperature = setTemperature + 0.5;
    Serial.print(server.arg("CONSIGNE_UP"));
    saveSetTemperature(setTemperature);
    server.send ( 200, "text/html", getPage() );
  } 
  else if ( server.hasArg("CONSIGNE_DOWN") ) {
    setTemperature = setTemperature - 0.5;
    Serial.print(server.arg("CONSIGNE_DOWN"));
    saveSetTemperature(setTemperature);
    server.send ( 200, "text/html", getPage() );
  } 
  else {*/
    serverWeb.send ( 200, "text/html", getPage() );
  //}  
}

void handleConsigneUp()
{
   setTemperature = setTemperature + 0.5;
   saveSetTemperature(setTemperature);
   serverWeb.send ( 200, "text/html", getPage() ); 
}

void handleConsigneDown()
{
   setTemperature = setTemperature - 0.5;
   saveSetTemperature(setTemperature);
   serverWeb.send ( 200, "text/html", getPage() ); 
}

void handleThresholdUp()
{
   tempThreshold = tempThreshold + 0.05;
   saveThresholdTemperature(tempThreshold);
   serverWeb.send ( 200, "text/html", getPage() ); 
}

void handleThresholdDown()
{
   tempThreshold = tempThreshold - 0.05;
   saveThresholdTemperature(tempThreshold);
   serverWeb.send ( 200, "text/html", getPage() ); 
}

void handlePointeOff()
{
   indModePointe = false;
   serverWeb.send ( 200, "text/html", getPage() ); 
}

void handlePointeOn()
{
   indModePointe = true;
   serverWeb.send ( 200, "text/html", getPage() ); 
}

//*****************************************************
// setup
//
//*****************************************************
void setup() {
  float eeSetTemperature;
  float eeTempThreshold;
  int wifiRetry = 0;

  Serial.begin(9600);

  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT);
  
  pinMode(RELAYPIN, OUTPUT);

  timeClient.begin();
  modePointe(false);
  
  //Start up the library
  sensors_floor.begin();
  dht.begin();

  //Read EEPROM
  EEPROM.begin(8);

  //Si une temperature a été enregistrée dans la mémoire, la ramener et la mettre dans SET
  EEPROM.get(EEaddress, eeSetTemperature);
  if (!isnan(eeSetTemperature))
  {
    setTemperature = eeSetTemperature;
  }
  Serial.print("setTemperature :");
  Serial.print(setTemperature);

  
  //Si une temperature a été enregistrée dans la mémoire, la ramener et la mettre dans SET
  EEPROM.get(EEaddress + sizeof(float), eeTempThreshold);
  if (!isnan(eeTempThreshold))
  {
    tempThreshold = eeTempThreshold;
  }
  Serial.print("SetTempThreshold :");
  Serial.print(tempThreshold);

  //Prepare the screen
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour

  tft.drawLine(0, 60, 240, 60, 0xBDF7); //Ligne du haut
  tft.drawLine(0, 200, 240, 200, 0xBDF7); //Ligne du bas

  tft.drawChar(125,10,210,7);
  flecheIn();
  flecheOut();  

  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  while ((WiFi.status() != WL_CONNECTED) && (wifiRetry <= 10)) {
    delay(900);
    wifiRetry = wifiRetry + 1;
    Serial.print(wifiRetry);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
  
  
    // On branche la fonction qui gère la premiere page / link to the function that manage launch page 
    serverWeb.on ( "/", handleRoot );
    serverWeb.on ( "/consigne_up", handleConsigneUp );
    serverWeb.on ( "/consigne_down", handleConsigneDown );
    serverWeb.on ( "/threshold_up", handleThresholdUp );
    serverWeb.on ( "/threshold_down", handleThresholdDown );
    serverWeb.on ( "/pointe_off", handlePointeOff );
    serverWeb.on ( "/pointe_on", handlePointeOn );
    
    // Start the server
    serverWeb.begin();
    Serial.println("Server started");
   
    // Print the IP address
    Serial.print("Use this URL : ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  }
//  ThingSpeak.begin( client );


}


//*****************************************************
// loop
//
//***************************************************** 
void loop() 
{
  unsigned long currentMillis = millis();

  serverWeb.handleClient();

  //Gestion des buttons Up et Down
  // read the state of the pushbutton value:
  buttonUpState = digitalRead(buttonUpPin);

  if (buttonUpState != lastButtonUpState)
  {
    if (buttonUpState == LOW)
    {
      Serial.println("UP - LOW");
      setTemperature = setTemperature + 0.5;
      affSetFloorTemp(setTemperature);
    }

    lastButtonUpState = buttonUpState;
    // Delay a little bit to avoid bouncing
    delay(50);
  }

  buttonDownState = analogRead(buttonDownPin);
 
  if (buttonDownState != lastButtonDownState)
  {
    if (buttonDownState >= 1000)
    {
      Serial.println("DOWN - LOW");
      setTemperature = setTemperature - 0.5;
      affSetFloorTemp(setTemperature);
    }

    lastButtonDownState = buttonDownState;
    // Delay a little bit to avoid bouncing
    delay(50);
  }


  //Vérifier l'état de la pompe à toutes les "pumpInterval"
  //Mettre à jour les infos sur l'écran
  //Non-Blocking
  if (currentMillis - previousMillis >= pumpInterval) 
  {
    //Traitement en mode de pointe
    modePointe(indModePointe);
    
    // save the last time you check the pump
    previousMillis = currentMillis;

    sensors_floor.requestTemperatures(); //Obtenir la temperature des DS18b
    floorTemp = sensors_floor.getTempC(adrFloorSensor);
    //Gestion de la pompe
    pump(floorTemp);

    //Get and Update infos on screen @ pumpInterval
    inTemp  = sensors_floor.getTempC(adrInSensor);
    outTemp = sensors_floor.getTempC(adrOutSensor);
    // Read temperature as Celsius (the default)
    roomTemp = dht.readTemperature() - 1;
    roomHum  = dht.readHumidity();

    // Check if any reads failed and exit early (to try again).
    if (isnan(roomTemp) || isnan(roomHum)) 
    {
      Serial.println("Failed to read from DHT sensor!");
    }


 /*   
Serial.println("inTemp:");
Serial.println(inTemp);

Serial.println("outTemp:");
Serial.println(outTemp);

Serial.println("roomTemp:");
Serial.println(roomTemp);

Serial.println("roomHum:");
Serial.println(roomHum);


Serial.println("floorTemp:");
Serial.println(floorTemp);


Serial.println("setTemperature:");
Serial.println(setTemperature);
*/
    //Mise à jour de l'affichage
    tempIn(inTemp);
    tempOut(outTemp);
    delta(inTemp, outTemp);
    affRoomTemp(roomTemp);
    affRoomHumidity(roomHum);
    affSetFloorTemp(setTemperature);
    affFloorTemp(floorTemp);
/*
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    write2TSData(channelID, 1, sensors_floor.getTempCByIndex(0), 
                            2, t,
                            3, h,
                            4, pumpState);
*/
    timeClient.update(); //Il a en plus son propre délai dans la librairie
    Serial.println(timeClient.getFormattedTime());
  }  

  //L'affichage de l'indicateur de pompe à son propre "non blocking thread"
  if (pumpState == true)
  {
    //non-blocking
    if (currentMillis - previousPumpIconMillis >= 1000) 
    {
      // save the last time you check the pumpÎcon
      previousPumpIconMillis = currentMillis;

      //Effacer l'icon
      tft.fillRect(100, 5, 50, 50, 0x0000);
     
      if ( lastPumpIcon == 2 )
      {
        tft.drawBitmap(100, 5, pumpIcon_1, 50, 50, 0xBDF7);  //0xBDF7
        lastPumpIcon = 1;
      }
      else
      {
        tft.drawBitmap(100, 5, pumpIcon_2, 50, 50, 0xBDF7);
        lastPumpIcon = 2;
      }
    }
  }
  else
  {
    //Effacer l'icon
    tft.fillRect(100, 5, 50, 50, 0x0000);
  }
}

//*****************************************************
// pump
// Gestion de l'activation de la pompe selon la consigne
//*****************************************************
void pump(float p_floorTemp)
{
  //Si pas en pause pointe
  if (pausePointe == false)
  {
    //Si la pompe est arrêtée et que le plancher à atteind la température la plus froid
    if ((pumpState == false) and (p_floorTemp <= ((setTemperature + overShoot) - tempThreshold)))
    {
      //Start pump
      digitalWrite(RELAYPIN, HIGH);
      //relay ON
      pumpState = true;
    }
  
    if ((pumpState == true) and (p_floorTemp >= (setTemperature + overShoot)))
    {
      //Stop pump
      //relay OFF
      digitalWrite(RELAYPIN, LOW);
      pumpState = false;    
    }
  }
  //Si en pause pointe, arrêter la pompe
  else
  {
    digitalWrite(RELAYPIN, LOW);
    pumpState = false;
  }
  
  Serial.println("État de la pompe");
  Serial.println(pumpState);
  Serial.println(p_floorTemp);
}

//*****************************************************
// saveSetTemperature
//
//*****************************************************
void saveSetTemperature(float p_temperature)
{
  EEPROM.put(EEaddress, p_temperature); //Sauvegarder la température sélectionnée
  EEPROM.commit();  
}

//*****************************************************
// saveThresholdTemperature
//
//*****************************************************
void saveThresholdTemperature(float p_temperature)
{
  EEPROM.put(EEaddress + sizeof(float), p_temperature); //Sauvegarder la température sélectionnée
  EEPROM.commit();  
}


//*****************************************************
// flecheIn
//
//*****************************************************
void flecheIn()
{
  tft.drawLine( 10, 205, 10, 230, TFT_BLUE);
  tft.drawLine( 5, 220, 10, 230, TFT_BLUE);
  tft.drawLine( 15, 220, 10, 230, TFT_BLUE);
  tft.drawCentreString("in", 10, 233, 1);
}

//*****************************************************
// flecheOut
//
//*****************************************************
void flecheOut()
{
  tft.drawLine( 230, 205, 230, 230, TFT_RED);
  tft.drawLine( 230, 205, 225, 215, TFT_RED);
  tft.drawLine( 230, 205, 235, 215, TFT_RED);
  tft.drawCentreString("out", 230, 233, 1);
}

//*****************************************************
// tempIn
//
//*****************************************************
void tempIn(float p_tempIn)
{
  tft.setTextColor(0x85B3, 0x0000);
  tft.drawFloat(p_tempIn, 1, 20, 213, 2);
  tft.setCursor(50, 213, 2);
  tft.print("`");
  tft.print("C");
  
}

//*****************************************************
// tempOut
//
//*****************************************************
void tempOut(float p_tempOut)
{
  tft.setTextColor(0x85B3, 0x0000);
  tft.drawFloat(p_tempOut, 1, 180, 213, 2);
  tft.setCursor(210, 213, 2);
  tft.print("`");
  tft.print("C");
}

//*****************************************************
// delta
//
//*****************************************************
void delta(float p_tempIn, float p_tempOut)
{
  tft.setTextColor(0xBDF7, 0x0000);
  tft.drawFloat(p_tempOut - p_tempIn, 1, 91, 210, 4);
  tft.setCursor(145, 210, 2);
  tft.print("`");
  tft.print("C");
}

//*****************************************************
// roomTemp
//
//*****************************************************
void affRoomTemp(float p_roomTemp)
{
  tft.setTextColor(0xBDF7, 0x0000);
  tft.drawFloat(p_roomTemp, 1, 5, 80, 8);
  //tft.getCursorX();
  tft.setCursor(200, 80, 4);
  tft.print("`");
  tft.print("C");
}

//*****************************************************
// roomHumidity
//
//*****************************************************
void affRoomHumidity(float p_roomHum)
{
  tft.setTextColor(0xBDF7, 0x0000);
  tft.drawFloat(p_roomHum, 1, 160, 170, 4);
  tft.setCursor(215, 170, 2);
  tft.print("%");
}

//*****************************************************
// setFloorTemp
//
//*****************************************************
void affSetFloorTemp(float p_setFloorTemp)
{
  tft.setTextColor(0xBDF7, 0x0000);
  tft.drawFloat(p_setFloorTemp, 1, 10, 15, 4);
  tft.setCursor(65, 15, 2);
  tft.print("`");
  tft.print("C");
  
}

//*****************************************************
// floorTemp
//
//*****************************************************
void affFloorTemp(float p_floorTemp)
{
  tft.setTextColor(0xBDF7, 0x0000);
  tft.drawFloat(p_floorTemp, 1, 166, 15, 4);
  tft.setCursor(220, 15, 2);
  tft.print("`");
  tft.print("C");
}


//*****************************************************
// modePointe
//
//*****************************************************
void modePointe(bool p_Pointe)
{
  int hhmm;

  //Si le mode pointe est activé
  if (p_Pointe == true)
  {
    //1623, 956
    hhmm = (timeClient.getHours() * 100) + timeClient.getMinutes();

    //1 heure avant une pointe, augmenter le set point de 1 degré
    if (((hhmm >= 500) && (hhmm <= 600)) || ((hhmm >= 1500) && (hhmm <= 1600)))
    {
      overShoot = const_overShoot;
    }
    else
    {
      overShoot = 0.0;
    }
  
    //Si on est dans l'intervalle de pointe
    if (((hhmm >= 600) && (hhmm <= 900)) || ((hhmm >= 1600) && (hhmm <= 2000)))
    {
      //Mettre à pause
      pausePointe = true;
    }
    else
    {
      pausePointe = false;
    }
  }
  else
  {
    pausePointe = false;
    overShoot   = 0.0;
  }
  
}

//*****************************************************
// write2TSData
//
//*****************************************************
//use this function if you want multiple fields simultaneously
/*int write2TSData( long TSChannel, unsigned int TSField1, float field1Data, 
                                  unsigned int TSField2, float field2Data,
                                  unsigned int TSField3, float field3Data,
                                  unsigned int TSField4, boolean field4Data)
{
  ThingSpeak.setField( TSField1, field1Data );
  ThingSpeak.setField( TSField2, field2Data );
  ThingSpeak.setField( TSField3, field3Data );
  ThingSpeak.setField( TSField4, field4Data );
   
  int writeSuccess = ThingSpeak.writeFields( TSChannel, writeAPIKey );
  return writeSuccess;
}*/
