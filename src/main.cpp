#include <ArduinoJson.h>
#include "data.h"

void setup() {

  pinInitilization();

  Serial.begin(115200);
  timer.startTimer(5000);

  //-------------------------------- Wifi connexion ------------------------

  connectAP();

  //-------------------------------- JWT token ------------------------
  
  if (obtainTokens()) {
    Serial.println("Authentification réussie. Jetons obtenus.");
  } else {
    Serial.println("Échec de l'authentification.");
  }
  
  //-------------------------------- I2C BMP280 communication ------------------------

  bmp_status = bmp.begin(0x76);
  while (!bmp_status) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring");
    timer.startTimer(3000);
  }
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    if (accessToken == "") {
      if (!obtainTokens()) {
        Serial.println("Erreur lors de l'obtention des jetons.");
        timer.startTimer(5000);
        return;
      }
    }

    HTTPClient http;
    http.begin(endPoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + accessToken);

    // Désactivez temporairement le Wi-Fi
    WiFi.mode(WIFI_OFF);
    timer.startTimer(100);
  
    // Construire les données JSON à envoyer
    StaticJsonDocument<100> jsonBuffer;

    //float temperature = getData(0, 40);
    float temperature = getTemperaturedata();
    fanManagement(temperature);

    /*jsonBuffer["quantityCO2"] = (int) getCO_CJMCU_6814_Data();
    jsonBuffer["quantityNH3"] = (int) getNH3_CJMCU_6814_Data();
    jsonBuffer["quantityNO2"] = (int) getNO2_CJMCU_6814_Data();
    jsonBuffer["quantityO3"] = (int) getNO2_CJMCU_6814_Data();
    jsonBuffer["quantityCO"] = (int) getCarbonMonoxideData();*/

    jsonBuffer["device_name"] = "ESP32";
    jsonBuffer["quantityCO2"] = getData(300, 500);
    jsonBuffer["quantityNH3"] = getData(50, 70);
    jsonBuffer["quantityNO2"] = getData(60, 100);
    jsonBuffer["quantityO3"] = getData(100, 150);
    jsonBuffer["quantityCO"] = getData(100, 200);
    jsonBuffer["fine_particle"] = (int) getFineParticleData();
    jsonBuffer["temperature"] = temperature;
    jsonBuffer["fan"] = fanState;

    // Réactivez le Wi-Fi
    connectAP();

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

  timer.startTimer(2000);
}