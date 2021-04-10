#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
//https://github.com/banalg/rooster.git
#include <porte.h>

const int ledPin = 2;
const int capteurLuminositePin = 34;
const int capteurPresencePin = 12;
const int resetButtonPin = 13;
const int buttonStopClosePin = 35;
const int buttonStopOpenPin = 32;
const int stepperMotorPins[] = {14, 27, 26, 25};

int doorStatus = 0; //1 : Openning, 2 : Closed, 0 : Wait

int valeurDelayLed = 1000;
bool etatLed = 0;
bool etatLedVoulu = 0;
int previousMillis = 0;

WiFiManager wm;
const char *ssid = "rooster";
const char *password = "rooster";
AsyncWebServer server(80);

Porte porte = Porte(stepperMotorPins, &buttonStopClosePin, &buttonStopOpenPin);



void blink(int nb, int delayms)
{
  for (int i = 0; i < nb; i++)
  {
    digitalWrite(ledPin, HIGH);
    delay(delayms);
    digitalWrite(ledPin, LOW);
    delay(delayms);
  }
}

void setup()
{
  //----------------------------------------------------Serial
  Serial.begin(115200);
  Serial.println("\n");
  Serial.println("Setup...");

  //----------------------------------------------------GPIO

  pinMode(ledPin, OUTPUT);
  pinMode(resetButtonPin, INPUT);
  blink(4, 250);

  pinMode(capteurLuminositePin, INPUT);
  pinMode(capteurPresencePin, INPUT);
  pinMode(buttonStopClosePin, INPUT);
  pinMode(buttonStopOpenPin, INPUT);

  // stepper mottor
  for (int i = 0; i < 4; i++)
  {
    pinMode(stepperMotorPins[i], OUTPUT);
  }

  //----------------------------------------------------SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("Erreur SPIFFS...");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }

  // ----------------------------------------------------WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Tentative de connexion Wifi...");
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect(ssid, password))
  {
    Serial.println("Erreur de connexion.");
  }
  else
  {
    Serial.println("Connexion etablie!");
  }


  /*
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\n");
  Serial.println("Connexion etablie!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
  */
  
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
    porte.ouvrir();
    request->send(204);
  });

  server.on("/closeDoor", HTTP_GET, [](AsyncWebServerRequest *request) {
    porte.fermer();
    request->send(204);
  });

  server.begin();


  Serial.println("Serveur actif!");

}


void loop()
{
  // Reset des configs de connexion
  if (digitalRead(resetButtonPin) == LOW)
  {
    Serial.println("Suppression des reglages Wifi et redemarrage...");
    wm.resetSettings();
    ESP.restart();
  }

  if (etatLedVoulu)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= valeurDelayLed)
    {
      previousMillis = currentMillis;

      etatLed = !etatLed;
      digitalWrite(ledPin, etatLed);
    }
  }
}
