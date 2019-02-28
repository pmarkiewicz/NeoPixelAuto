#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>

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

  String message = "Headers: ";
  message += server.headers();
  message += "\n";

  for (uint8_t i = 0; i < server.headers(); i++) {
    message += " " + server.headerName(i) + ": " + server.header(i) + "\n";
  }

  Serial.print(message);
}

void handleNotFound()
{
  Serial.printf("404: %s\n", server.uri().c_str());
  server.send(404, "text/plain", "404: Not found"); 
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
    Serial.printf("[%u] get Text: %s\n", num, payload);
    if (payload[0] == '#')
    {                                                                       // we get RGB data
      uint32_t rgb = (uint32_t)strtol((const char *)&payload[1], NULL, 16); // decode rgb data
      int r = ((rgb >> 16) & 0xFF);                                        // 10 bits per color, so R: bits 20-29
      int g = ((rgb >> 8) & 0xFF);                                        // G: bits 10-19
      int b = rgb & 0xFF;                                                  // B: bits  0-9

      Serial.printf("rgb: [%X] r [%u] g [%u] b [%u]\n", rgb, r, g, b);
      display_set_color(r, g, b);
    }
    break;
    default:
      Serial.printf("Unknown WS command\n");
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  display_init();

  //display_set_color(50, 50, 50);
  //Serial.print("Setting soft-AP ... ");
  boolean result = true;  //WiFi.softAP("ESPsoftAP_01", "12345678");

  WiFi.begin("coda", "olaola123");    
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print('.');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());  

  if (result)
  {
    Serial.println("Ready");
    display_set_color(10, 128, 10);

    server.on("/", HTTP_GET, handleRoot);
    server.onNotFound(handleNotFound); 
    server.begin();

    MDNS.begin("esp8266");
    webSocket.begin();                          // start the websocket server
    webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'

    Serial.println("WebSocket server started.");
  }
  else
  {
    Serial.println("Failed!");
    display_set_color(128, 10, 10);
  }
}

void loop()
{
  server.handleClient();
  MDNS.update();
  webSocket.loop();
}