#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include "DHT.h"
#include <EEPROM.h>
#include <ESP8266WebServer.h>
 
#define ONE_WIRE_PIN D2
#define RELAYPIN D1
#define DHTPIN D3     // what pin we're connected to

#define DHTTYPE DHT22

const char* ssid = "safe_local";
const char* password = "ew6NuaOIdLb3CKmCKS";
const long pumpInterval = 10000; //Check  @ 10seconds

//WiFiServer server(80); // Set web server port number to 80
ESP8266WebServer server ( 80 );
 
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors_floor(&oneWire);

DHT dht(DHTPIN, DHTTYPE, 11); // Initialize DHT sensor for normal 16mhz Arduino

float setTemperature = 15.0;  //10.0 C par défaut

float tempThreshold = 0.10; //combien de degré +/- avant de déclancher la pompe

unsigned long previousMillis = 0;

boolean pumpState = false;

// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
int EEaddress = 0;

String getPage(){
  sensors_floor.requestTemperatures(); //Obtenir la temperature du plancher
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

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
  page +=           sensors_floor.getTempCByIndex(0);
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
  page +=             sensors_floor.getTempCByIndex(0);
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
    server.send ( 200, "text/html", getPage() );
  //}  
}

void handleConsigneUp()
{
   setTemperature = setTemperature + 0.5;
   saveSetTemperature(setTemperature);
   server.send ( 200, "text/html", getPage() ); 
}

void handleConsigneDown()
{
   setTemperature = setTemperature - 0.5;
   saveSetTemperature(setTemperature);
   server.send ( 200, "text/html", getPage() ); 
}

void handleThresholdUp()
{
   tempThreshold = tempThreshold + 0.05;
   saveThresholdTemperature(tempThreshold);
   server.send ( 200, "text/html", getPage() ); 
}

void handleThresholdDown()
{
   tempThreshold = tempThreshold - 0.05;
   saveThresholdTemperature(tempThreshold);
   server.send ( 200, "text/html", getPage() ); 
}

//*****************************************************
// setup
//
//*****************************************************
void setup() {
  float eeSetTemperature;
  float eeTempThreshold;
  
  Serial.begin(9600);
  
  pinMode(RELAYPIN, OUTPUT);
  dht.begin();

  EEPROM.begin(8);

  //Si une temperature a été enregistrée dans la mémoire, la ramener et la mettre dans SET
  EEPROM.get(EEaddress, eeSetTemperature);
  if (!isnan(eeSetTemperature))
  {
    setTemperature = eeSetTemperature;
  }
  Serial.print("Set :");
  Serial.print(setTemperature);

  
  //Si une temperature a été enregistrée dans la mémoire, la ramener et la mettre dans SET
  EEPROM.get(EEaddress + sizeof(float), eeTempThreshold);
  if (!isnan(eeTempThreshold))
  {
    tempThreshold = eeTempThreshold;
  }
  Serial.print("Set :");
  Serial.print(tempThreshold);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");


  // On branche la fonction qui gère la premiere page / link to the function that manage launch page 
  server.on ( "/", handleRoot );
  server.on ( "/consigne_up", handleConsigneUp );
  server.on ( "/consigne_down", handleConsigneDown );
  server.on ( "/threshold_up", handleThresholdUp );
  server.on ( "/threshold_down", handleThresholdDown );
  
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}


//*****************************************************
// loop
//
//***************************************************** 
void loop() 
{
  unsigned long currentMillis = millis();

  server.handleClient();

  //Vérifier l'état de la pompe à toutes les "pumpInterval"
  //Non-Blocking
  if (currentMillis - previousMillis >= pumpInterval) 
  {
    // save the last time you check the pump
    previousMillis = currentMillis;

    pump();
  }

}

//*****************************************************
// pump
//
//*****************************************************
void pump()
{
  sensors_floor.requestTemperatures(); //Obtenir la temperature du plancher

  //Si la pompe est arrêtée et que le plancher à atteind la température la plus froid
  if ((pumpState == false) and (sensors_floor.getTempCByIndex(0) < (setTemperature - tempThreshold)))
  {
    //Start pump
     digitalWrite(RELAYPIN, HIGH);
    //relay ON
    pumpState = true;
  }

  if ((pumpState == true) and (sensors_floor.getTempCByIndex(0) > setTemperature))
  {
    //Stop pump
    //relay OFF
    digitalWrite(RELAYPIN, LOW);
    pumpState = false;    
  }

  Serial.println("État de la pompe");
  Serial.println(pumpState);
  Serial.println(sensors_floor.getTempCByIndex(0));
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

