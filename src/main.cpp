#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>      
#include <ArduinoOTA.h>

#include "display.h"

#include "html_inside/html_begin.pp" // header from this repo
#include "main_page.html"            // your HTML file
#include "html_inside/html_end.pp"   // header from this repo

WiFiClient wifi;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

void handleRoot()
{
  Serial.printf("Main page req, [%u] bytes\n", strlen(html_page));
  Serial.printf("404: %s\n", server.uri().c_str());
  server.send(200, "text/html", html_page);
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
    int r = ((rgb >> 20) & 0x3FF);                                        // 10 bits per color, so R: bits 20-29
    int g = ((rgb >> 10) & 0x3FF);                                        // G: bits 10-19
    int b = rgb & 0x3FF;                                                  // B: bits  0-9

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

void startWiFi() { // Try to connect to some given access points. Then wait for a connection
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  wifiManager.autoConnect("neopixel");

  Serial.println("WiFi connected");
}

void startOTA() {
  ArduinoOTA.setHostname("neopixel");
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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
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
  MDNS.begin("neopixel");
}

void startLED()
{
  display_init();

  display_off();
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  startLED();

  startWiFi();
  startOTA();

  startServer();
  startWebsocket();
  //startMDNS();
}

void loop()
{
  server.handleClient();
  //MDNS.update();
  webSocket.loop();
}