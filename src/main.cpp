#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "time.h"

#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include <porte.h>

#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"
#define LED_BUILT_IN 2

const int ledPin = 2;
const int capteurLuminositePin = 34;
const int capteurPresencePin = 12;
const int resetButtonPin = 13;
const int buttonStopClosePin = 35;
const int buttonStopOpenPin = 32;
const int stepperMotorPins[] = {14, 15, 33, 32};

struct tm openTime;
struct tm closeTime;

int doorStatus = 0; //1 : Openning, 2 : Closed, 0 : Wait

int valeurDelayLed = 1000;
bool etatLed = 0;
bool etatLedVoulu = 0;
int previousMillis = 0;

WiFiManager wm;
const char *AP_ssid = "rooster";
AsyncWebServer server(80);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// Initialize Telegram BOT
String BOTtoken = "1708178479:AAFS0L0yDOZVSrzTJNHcm8B4jgnB-bhVmCs"; // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
String CHAT_ID = "-572800284";

bool sendPhoto = false;

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

Porte porte = Porte(stepperMotorPins, &buttonStopClosePin, &buttonStopOpenPin);

void printLocalTime();

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

//Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
camera_config_t config;

void cameraSetup() {
	camera_config_t config;
	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;
	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;
	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;
	config.pin_sscb_sda = SIOD_GPIO_NUM;
	config.pin_sscb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;
	config.xclk_freq_hz = 20000000;
	config.pixel_format = PIXFORMAT_JPEG;

	psramFound();
	config.frame_size = FRAMESIZE_QVGA;
	config.jpeg_quality = 10;
	config.fb_count = 1;

	// camera init
	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK) {
		Serial.printf("Camera init failed with error 0x%x", err);
		return;
	}
	Serial.println("Camera configuration complete!");
}

void handleNewMessages(int numNewMessages)
{
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID)
    {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;
    if (text == "/start")
    {
      String welcome = "Welcome , " + from_name + "\n";
      welcome += "Use the following commands to interact with the ESP32-CAM \n";
      welcome += "/photo : takes a new photo\n";
      welcome += "/flash : toggles flash LED \n";
      bot.sendMessage(CHAT_ID, welcome, "");
    }
    /*if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
      Serial.println("Change flash LED state");
    }*/
    if (text == "/photo")
    {
      sendPhoto = true;
      Serial.println("New photo request");
    }
  }
}

String sendPhotoTelegram()
{
  const char *myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }

  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443))
  {
    Serial.println("Connection successful");

    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;

    clientTCP.println("POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024)
    {
      if (n + 1024 < fbLen)
      {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0)
      {
        size_t remainder = fbLen % 1024;
        clientTCP.write(fbBuf, remainder);
      }
    }

    clientTCP.print(tail);

    esp_camera_fb_return(fb);

    int waitTime = 10000; // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + waitTime) > millis())
    {
      Serial.print(".");
      delay(100);
      while (clientTCP.available())
      {
        char c = clientTCP.read();
        if (state == true)
          getBody += String(c);
        if (c == '\n')
        {
          if (getAll.length() == 0)
            state = true;
          getAll = "";
        }
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length() > 0)
        break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else
  {
    getBody = "Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  //----------------------------------------------------Serial
  Serial.begin(115200);
  Serial.println("\n");
  Serial.println("Setup...");

  //----------------------------------------------------GPIO

  pinMode(ledPin, OUTPUT);
  pinMode(resetButtonPin, INPUT);
  blink(10, 20);

  pinMode(capteurLuminositePin, INPUT);
  pinMode(capteurPresencePin, INPUT);
  pinMode(buttonStopClosePin, INPUT);
  pinMode(buttonStopOpenPin, INPUT);

  // stepper mottor
  for (int i = 0; i < 4; i++)
  {
    pinMode(stepperMotorPins[i], OUTPUT);
  }

  //----------------------------------------------------Config and init the camera
  cameraSetup();

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
  //WiFi.begin(AP_ssid, AP_password);
  Serial.print("Tentative de connexion Wifi...");
  wm.setConfigPortalTimeout(180);
  wm.setHttpPort(81);
  wm.setHostname("rooster");
  if (!wm.autoConnect(AP_ssid))
  {
    Serial.println("Erreur de connexion.");
  }
  else
  {
    Serial.println("Connexion etablie!");
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();
    clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  }

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

  server.on("/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
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

  server.on("/sendPhotoTelegram", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendPhoto = true;
    sendPhotoTelegram();
    request->send(204);
  });

  server.begin();

  Serial.println("Serveur actif !");
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
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

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  Serial.println();
}
