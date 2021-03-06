#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#ifdef USE_MQTT
  #include <PubSubClient.h>
#endif

#include "display.h"
#include "ntp_time.h"

const char* hostname = "neopixel";
const char* ota_pwd ="esp8266";

/************* MQTT CONFIGURATION (if used, change these topics as you wish)  **************************/
#ifdef USE_MQTT
  const char* mqtt_server = "";
  const int mqtt_port = 1883;
  const char* mqtt_username = NULL;
  const char* mqtt_password = NULL;

/************* MQTT TOPICS (change these topics as you wish)  **************************/
const char *light_state_topic = "hs/neopixel";
const char *light_set_topic = "hs/neopixel/set";
#endif

const char *on_cmd = "ON";
const char *off_cmd = "OFF";
const char *color = "color";

const char* index_html = "/index.html";
const char* favico = "/favicon.ico";

WiFiClient wifi;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

static int r = 0;
static int g = 0;
static int b = 0;

#ifdef USE_MQTT
  PubSubClient mqttClient(wifi);
  bool mqttInUse = false;
#endif

void handleRoot()
{
  SPIFFS.begin();

  File file = SPIFFS.open(index_html, "r");
  server.streamFile(file, "text/html");
  file.close();

  SPIFFS.end();
}

void handleTime() 
{
  static char* tm = (char*)"    ";

  unsigned char h;
  unsigned char m;

  get_time(h, m);

  tm[0] = '0' + h / 10;
  tm[1] = '0' + h % 10;
  tm[2] = '0' + m / 10;
  tm[3] = '0' + m % 10;

  server.send(200, "text/plain", tm);
}

void handleFavico()
{
  SPIFFS.begin();

  File file = SPIFFS.open(favico, "r");
  server.streamFile(file, "image/x-icon");
  file.close();

  SPIFFS.end();
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found");
}

void handleWSText(uint8_t num, uint8_t *payload)
{
  //Serial.printf("[%u] get Text: %s\n", num, payload);
  if (payload[0] == '#')
  {                                                                       // we get RGB data
      uint32_t rgb = (uint32_t)strtol((const char *)&payload[1], NULL, 16); // decode rgb data
      r = ((rgb >> 16) & 0xFF);                                        
      g = ((rgb >> 8) & 0xFF);                                       
      b = rgb & 0xFF;                                             
  }
  else if (payload[0] == '?')
  {
      char buf = "#000000";

      webSocket.sendTXT(num, "#000000"); 
    // send(rgb);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{ // When a WebSocket message is received
  switch (type)
  {
  case WStype_DISCONNECTED: // if the websocket is disconnected
    #ifdef _DEBUG
      Serial.printf("[%u] Disconnected!\n", num);
    #endif
    break;
  case WStype_ERROR:
    #ifdef _DEBUG
      Serial.printf("[%u] Error!\n", num);
    #endif
    break;
  case WStype_CONNECTED:
      #ifdef _DEBUG
      { // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      #endif
    break;
  case WStype_TEXT: // if new text data is received
    handleWSText(num, payload);
    break;
  default:
    #ifdef _DEBUG
      Serial.printf("Unknown WS command\n");
    #endif
    break;
  }
}

#ifdef USE_MQTT
  void mqttCallback(char *topic, byte *payload, unsigned int length)
  {
    #ifdef _DEBUG
      Serial.print("Message arrived [");
      Serial.print(topic);
      Serial.print("] ");
    #endif
  }

  void mqttReconnect()
  {
    // Loop until we're reconnected
    while (!mqttClient.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (mqttClient.connect("ESP8266Client"))
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        mqttClient.publish(light_state_topic, "hello world");
        // ... and resubscribe
        mqttClient.subscribe(light_state_topic);
      }
      else
      {
        #ifdef _DEBUG
          Serial.print("failed, rc=");
          Serial.print(mqttClient.state());
          Serial.println(" try again in 5 seconds");
        #endif
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }
#endif

void startWiFi()
{
  WiFiManager wifiManager;

  wifiManager.setConfigPortalTimeout(120);
  wifiManager.setTimeout(20);

  if (!wifiManager.autoConnect(hostname))
  {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  #ifdef _DEBUG
    Serial.println("Connected to Wifi");
  #endif
}

#ifdef USE_MQTT
  bool isMQTTConfigured() 
  {
    return mqtt_server && strlen(mqtt_server) > 0;
  }

  void startMQTT()
  {
    if (isMQTTConfigured())
    {
      mqttClient.setServer(mqtt_server, mqtt_port);
      mqttClient.setCallback(mqttCallback);
      mqttClient.connect("neopixel", mqtt_username, mqtt_password);

      mqttInUse = true;
    }
  }
#endif

void startOTA()
{
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(ota_pwd);

  ArduinoOTA.onStart([]() {
    #ifdef _DEBUG
      Serial.println("Start");
    #endif
    display_yellow();
  });

  ArduinoOTA.onEnd([]() {
    #ifdef _DEBUG
      Serial.println("\nEnd");
    #endif
    display_off();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef _DEBUG
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif

    display_yellow();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    display_red();

    #ifdef _DEBUG
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)
        Serial.println("End Failed");
    #endif
  });

  ArduinoOTA.begin();
}

void startServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/tm", HTTP_GET, handleTime);
  server.on(favico, HTTP_GET, handleFavico);
  server.onNotFound(handleNotFound);
  server.begin();
}

void startWebsocket()
{
  webSocket.begin();                 // start the websocket server
  webSocket.onEvent(webSocketEvent); // if there's an incomming websocket message, go to function 'webSocketEvent'
}

void startLED()
{
  display_init();

  display_off();
}

void setup()
{
  #ifdef _DEBUG
    Serial.begin(115200);
  #endif

  delay(10);

  startLED();

  startWiFi();
  startOTA();

  ntp_time_init();

  startServer();
  startWebsocket();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    startWiFi();

    return;
  }

  server.handleClient();
  webSocket.loop();
  ArduinoOTA.handle();
  
  #ifdef USE_MQTT
  if (mqttInUse)
  {
    if (!mqttClient.connected())
    {
      mqttReconnect();
    }
    mqttClient.loop();
  }
  #endif

  if (millis() % 1000 == 0) {
    ntp_time_update();

    unsigned char hr;
    unsigned char min;
    get_time(hr, min);
  
    display_set_color(r, g, b);

    if (hr == 2 && min == 38) {
      r = 0;
      g = 0;
      b = 0;
    }
  }
}