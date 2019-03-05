#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "display.h"
#include "config.h"

//#include "html_inside/html_begin.pp" // header from this repo
//#include "main_page.html"            // your HTML file
//#include "html_inside/html_end.pp"   // header from this repo

/************* MQTT TOPICS (change these topics as you wish)  **************************/
const char *light_state_topic = "hs/neopixel";
const char *light_set_topic = "hs/neopixel/set";

const char *on_cmd = "ON";
const char *off_cmd = "OFF";
const char *color = "color";

const char* index_html = "/index.html";

const char* hostname = "neopixel";

WiFiClient wifi;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
PubSubClient mqttClient(wifi);
bool mqttInUse = false;

void handleRoot()
{
  SPIFFS.begin();

  File file = SPIFFS.open(index_html, "r");
  server.streamFile(file, "text/html");
  file.close();

  SPIFFS.end();
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found");
}

void handleWSText(uint8_t num, uint8_t *payload)
{
  Serial.printf("[%u] get Text: %s\n", num, payload);
  if (payload[0] == '#')
  {                                                                       // we get RGB data
      uint32_t rgb = (uint32_t)strtol((const char *)&payload[1], NULL, 16); // decode rgb data
      int r = ((rgb >> 16) & 0xFF);                                        
      int g = ((rgb >> 8) & 0xFF);                                       
      int b = rgb & 0xFF;                                             

      display_set_color(r, g, b);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{ // When a WebSocket message is received
  switch (type)
  {
  case WStype_DISCONNECTED: // if the websocket is disconnected
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_ERROR:
    Serial.printf("[%u] Error!\n", num);
    break;
  case WStype_CONNECTED:
  { // if a new websocket connection is established
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    //rainbow = false; // Turn rainbow off when a new connection is established
  }
  break;
  case WStype_TEXT: // if new text data is received
    handleWSText(num, payload);
    break;
  default:
    Serial.printf("Unknown WS command\n");
    break;
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
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
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void startWiFi()
{
  char no_of_leds[4]; // max uint8
  char mqtt_port[6];  // max uint16

  itoa(config.no_of_leds, no_of_leds, 10);
  itoa(config.mqtt_port, mqtt_port, 10);

  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", config.mqtt_server, sizeof(config.mqtt_server));
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, sizeof(mqtt_port));
  WiFiManagerParameter custom_mqtt_username("username", "mqtt username", config.mqtt_username, sizeof(config.mqtt_username));
  WiFiManagerParameter custom_mqtt_password("pwd", "mqtt pwd", config.mqtt_password, sizeof(config.mqtt_password));
  WiFiManagerParameter custom_no_of_leds("leds", "no of leds", no_of_leds, sizeof(no_of_leds));

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_no_of_leds);
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.setTimeout(20);

  if (!wifiManager.autoConnect(hostname))
  {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Serial.println("Connected to Wifi");
  strlcpy(config.mqtt_server, custom_mqtt_server.getValue(), sizeof(config.mqtt_server));
  strlcpy(config.mqtt_server, custom_mqtt_server.getValue(), sizeof(config.mqtt_server));
  strlcpy(config.mqtt_username, custom_mqtt_username.getValue(), sizeof(config.mqtt_username));
  
  config.mqtt_port = (uint16_t)atoi(custom_mqtt_port.getValue());
  config.no_of_leds = (uint8_t)atoi(custom_no_of_leds.getValue());
}

bool isMQTTConfigured() 
{
  return config.mqtt_server && strlen(config.mqtt_server) > 0;
}

void startMQTT()
{
  if (isMQTTConfigured())
  {
    mqttClient.setServer(config.mqtt_server, config.mqtt_port);
    mqttClient.setCallback(mqttCallback);
    mqttClient.connect("neopixel", config.mqtt_username, config.mqtt_password);

    mqttInUse = true;
  }
}

void startOTA()
{
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword("esp8266");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    display_yellow();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    display_off();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    display_yellow();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    display_red();
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
  });

  ArduinoOTA.begin();
}

void startServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
}

void startWebsocket()
{
  webSocket.begin();                 // start the websocket server
  webSocket.onEvent(webSocketEvent); // if there's an incomming websocket message, go to function 'webSocketEvent'
}

void startMDNS()
{
  MDNS.begin(hostname);
}

void startLED()
{
  display_init(config.no_of_leds);

  display_off();
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  config_load();

  startLED();

  startWiFi();
  startOTA();

  startServer();
  startWebsocket();
  startMDNS();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    startWiFi();

    return;
  }

  server.handleClient();
  MDNS.update();
  webSocket.loop();
  ArduinoOTA.handle();
  
  if (mqttInUse)
  {
    if (!mqttClient.connected())
    {
      mqttReconnect();
    }
    mqttClient.loop();
  }
}