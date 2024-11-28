#include <WiFi.h>
#include <SPI.h>
#include <HTTPClient.h>
#include "credentials.h"

void connectAP()
{
    Serial.println("Connect to my WiFi");
    WiFi.begin(ssid, password);
    byte count = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        count++;

        if(count > 30)
        {
            ESP.restart();
        }
    }

  Serial.println("\nConnecté au WiFi");
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
