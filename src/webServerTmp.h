/*
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class webServer
{

  webServer()
  {
  }

public:
  void connect(char *ssid, char *password)
  {
    WiFi.begin(ssid, password);
    Serial.print("Tentative de connexion Wifi...");

    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(100);
    }

    Serial.println("\n");
    Serial.println("Connexion etablie!");
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.localIP());
  }

void initWebServer(){



  //----------------------------------------------------SERVER
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/w3.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/jquery-3.4.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/jquery-3.4.1.min.js", "text/javascript");
  });

  server.on("/lireLuminosite", HTTP_GET, [](AsyncWebServerRequest *request) {
    int val = analogRead(capteurLuminositePin);
    String luminosite = String(val);
    request->send(200, "text/plain", luminosite);
  });

  server.on("/openDoor", HTTP_GET, [](AsyncWebServerRequest *request) {
    etatLedVoulu = 1;
    request->send(204);
  });

  server.on("/closeDoor", HTTP_GET, [](AsyncWebServerRequest *request) {
    etatLedVoulu = 0;
    digitalWrite(ledPin, LOW);
    etatLed = 0;
    request->send(204);
  });

  server.begin();
  }
}
*/