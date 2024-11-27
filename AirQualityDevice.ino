#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "MQ131.h"
#include <ArduinoJson.h>
#include <string.h> 
//#include "DustSensor.h"

//--------------------- Define for the dust_sensor (PM2.5) ------------------------

#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 400            //mv
#define        SYS_VOLTAGE                     5000    


//--------------------------------- Pin definition ---------------------------------
#define        DUST_ILED_PIN                   26 
#define        DUST_DATA_PIN                   25

/*#define        MQ131_D_PIN                     0
#define        MQ131_A_PIN                     1*/

#define        MQ7_A_PIN                       32
#define        MQ7_D_PIN                       33

#define        CJMCU_6814_CO                   5
#define        CJMCU_6814_NH3                  18
#define        CJMCU_6814_NO2                  19

#define        FAN_PIN                         15 
/*#define        BUTTON_PIN                      6

#define        ledPin                          2*/

const char* ssid = "UNIFI_IDO2";
const char* password = "99Bidules!";

/*const char* ssid = "238lavigne";
const char* password = "238lavigne";*/

const char* authUrl = "http://192.168.20.139:8001/api/token/";
const char* endPoint = "http://192.168.20.139:8001/api/data/";

/*const char* authUrl = "http://192.168.2.23:8001/api/token/";
const char* endPoint = "http://192.168.2.23:8001/api/data/";*/

String accessToken;
String refreshToken;

// Informations d'identification
const char* username = "jospin"; // Remplacez par votre nom d'utilisateur
const char* user_password = "admin"; // Remplacez par votre mot de passe

const float MAX_VOLTS = 5.0;
const float MAX_ANALOG_STEPS = 1023.0;

Adafruit_BMP280 bmp; 
unsigned bmp_status;

int fanState = 0;

/*
  Pin initialization
*/
void pinInitilization(){
  pinMode(DUST_ILED_PIN, OUTPUT);
  pinMode(MQ7_D_PIN, OUTPUT);
  pinMode(MQ7_A_PIN, INPUT);
  pinMode(CJMCU_6814_CO, INPUT);
  pinMode(CJMCU_6814_NH3, INPUT);
  pinMode(CJMCU_6814_NO2, INPUT);
  pinMode(FAN_PIN, OUTPUT);

  digitalWrite(DUST_ILED_PIN, LOW);
  /*pinMode(ledPin, OUTPUT);*/
}

void setup() {

  pinInitilization();

  Serial.begin(115200);
  delay(5000);

  //-------------------------------- Wifi connexion ------------------------

  WiFi.begin(ssid, password);
  Serial.print("Connexion au WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnecté au WiFi");

  //-------------------------------- JWT token ------------------------
  
  if (obtainTokens()) {
    Serial.println("Authentification réussie. Jetons obtenus.");
  } else {
    Serial.println("Échec de l'authentification.");
  }
  
  //-------------------------------- I2C BMP280 communication ------------------------

  bmp_status = bmp.begin(0x76);
  //bmp_status = bmp.begin(0, 4);
  while (!bmp_status) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring");
    delay(3000);
  }
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    if (accessToken == "") {
      if (!obtainTokens()) {
        Serial.println("Erreur lors de l'obtention des jetons.");
        delay(5000);
        return;
      }
    }

    HTTPClient http;
    http.begin(endPoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + accessToken);

    // Construire les données JSON à envoyer
    StaticJsonDocument<100> jsonBuffer;

    //float temperature = getTemperaturedata();
    float temperature = getData(0, 40);
    fanManagement(temperature);

    jsonBuffer["device_name"] = "ESP32";
    jsonBuffer["quantityCO2"] = getCO_CJMCU_6814_Data();
    jsonBuffer["quantityNH3"] = getNH3_CJMCU_6814_Data();
    jsonBuffer["quantityNO2"] = getNO2_CJMCU_6814_Data();
    jsonBuffer["quantityCO"] = getCarbonMonoxideData();
    jsonBuffer["fine_particle"] = getFineParticleData();
    jsonBuffer["temperature"] = temperature;
    jsonBuffer["fan"] = fanState;

    String jsonData;
    serializeJson(jsonBuffer, jsonData);

    // Afficher les données à envoyer pour vérification
    Serial.print("Données JSON : ");
    Serial.println(jsonData);
    Serial.println(getTemperaturedata());

    // Envoyer les données par requête POST
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode == 401) {  // Si le jeton d'accès est expiré

      Serial.println("Jeton expiré. Rafraîchissement en cours...");

      if (refreshAccessToken()) 
        Serial.println("Jeton rafraîchi avec succès.");
      else
        Serial.println("Erreur de rafraîchissement du jeton.");

    } else if (httpResponseCode > 0) {

      String response = http.getString();  // Lire la réponse du serveur
      Serial.print("Code de réponse : ");
      Serial.println(httpResponseCode);
      Serial.println("Réponse du serveur : ");
      Serial.println(response);

    } else {

      Serial.print("Erreur de connexion : ");
      Serial.println(httpResponseCode);

    }

    http.end();

  } else {
    Serial.println("WiFi non connecté.");
  }

  delay(2000);
}


int getData(int min, int max) {
  return min + (rand() % (max - min + 1));
}

/*
  CJMCU_6814 sensor
  Get carbon monoxide data
*/
float getCO_CJMCU_6814_Data(){
  return analogRead(CJMCU_6814_CO)* (MAX_VOLTS / MAX_ANALOG_STEPS);
}

/*
  CJMCU_6814 sensor
  Get carbon monoxide data
*/
float getNH3_CJMCU_6814_Data(){
  return analogRead(CJMCU_6814_NH3)* (MAX_VOLTS / MAX_ANALOG_STEPS);
}

/*
  CJMCU_6814 sensor
  Get carbon monoxide data
*/
float getNO2_CJMCU_6814_Data(){
  return analogRead(CJMCU_6814_NO2)* (MAX_VOLTS / MAX_ANALOG_STEPS);
}

/* 
  MQ7 sensor
  Get carbon monoxide data
*/
float getCarbonMonoxideData(){
  // A) preparation
  // turn the heater fully on
  analogWrite(MQ7_A_PIN, HIGH); // HIGH = 255
  // heat for 1 min
  delay(2000);
  // now reducing the heating power: turn the heater to approx 1,4V
  analogWrite(MQ7_A_PIN, 71.4); // 255x1400/5000
  // heat for 90 sec
  delay(5000);
  
  // B) reading    
  // CO2 via MQ7: we need to read the sensor at 5V, but must not let it heat up. So hurry!
  analogWrite(MQ7_A_PIN, HIGH); 

  return analogRead(MQ7_A_PIN);
}

/* 
  Dust sensor ou PM2.5 
  Get fine particle data 
*/
float getFineParticleData(){

  float density, voltage;
  int   adcvalue;
  
  digitalWrite(DUST_ILED_PIN, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(DUST_DATA_PIN); 
  digitalWrite(DUST_ILED_PIN, LOW);
  
  adcvalue = filter(adcvalue);

  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11; //covert voltage (mv)

  if(voltage >= NO_DUST_VOLTAGE) //voltage to density
  {
    voltage -= NO_DUST_VOLTAGE;
    
    density = voltage * COV_RATIO;
  }
  else
    density = 0;
 
  /*Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");  */

  return density;
}

/* 
  BMP280 sensor 
  Get temperature
*/
float getTemperaturedata(){
  return bmp.readTemperature();
}

/*
*/
void fanManagement(float temperature){
  if (temperature > 25 && temperature <= 30){
    analogWrite(FAN_PIN, 85);
    fanState = 1;
  }
  else if (temperature > 30 && temperature <= 40){
    analogWrite(FAN_PIN, 170);
    fanState = 1;
  }
  else if (temperature > 40){
    analogWrite(FAN_PIN, 255);
    fanState = 1;
  }
  else{
    analogWrite(FAN_PIN, 0);
    fanState = 0;
  }
}

// Fonction pour obtenir le jeton d'accès et le jeton de rafraîchissement
bool obtainTokens() {
  HTTPClient http;
  http.begin(authUrl);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"username\": \"" + String(username) + "\", \"password\": \"" + String(user_password) + "\"}";
  Serial.println("Json AUth : " + jsonData);
  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Réponse d'authentification : " + response);

    // Extraire les jetons (assurez-vous d'inclure la bibliothèque ArduinoJson si nécessaire)
    int accessTokenStart = response.indexOf("access\":\"") + 9;
    int accessTokenEnd = response.indexOf("\"", accessTokenStart);
    accessToken = response.substring(accessTokenStart, accessTokenEnd);

    int refreshTokenStart = response.indexOf("refresh\":\"") + 10;
    int refreshTokenEnd = response.indexOf("\"", refreshTokenStart);
    refreshToken = response.substring(refreshTokenStart, refreshTokenEnd);

    http.end();
    return true;
  } else {
    Serial.print("Erreur d'authentification, code : ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

// Fonction pour rafraîchir le jeton d'accès
bool refreshAccessToken() {
  HTTPClient http;
  String refreshUrl = String(authUrl) + "refresh/";
  http.begin(refreshUrl);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"refresh\": \"" + refreshToken + "\"}";
  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Réponse de rafraîchissement : " + response);

    int accessTokenStart = response.indexOf("access\":\"") + 9;
    int accessTokenEnd = response.indexOf("\"", accessTokenStart);
    accessToken = response.substring(accessTokenStart, accessTokenEnd);

    http.end();
    return true;
  } else {
    Serial.print("Erreur de rafraîchissement du jeton, code : ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

/*
private function
*/
int filter(int m)
{
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;
  
  if(flag_first == 0)
  {
    flag_first = 1;
    for(i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++)
    {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];
    
    i = sum / 10.0;
    return i;
  }
}
