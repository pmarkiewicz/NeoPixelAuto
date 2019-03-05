#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h>

#include "config.h"

config_struct config = {"", 1883, "", "", 8};

static void load_from_file(File &configFile)
{
  Serial.println("opened config file");
  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.parseObject(buf.get());
  json.printTo(Serial);
  if (json.success())
  {
    Serial.println("\nparsed json");

    strlcpy(config.mqtt_server, json["mqtt_server"] | "", sizeof(config.mqtt_server));
    config.mqtt_port = (uint16_t)atoi(json["mqtt_port"]);
    strlcpy(config.mqtt_username, json["mqtt_username"] | "", sizeof(config.mqtt_username));
    strlcpy(config.mqtt_password, json["mqtt_password"] | "", sizeof(config.mqtt_password));
    config.no_of_leds = (uint8_t)atoi(json["no_of_ledss"]);
  }
  else
  {
    Serial.println("failed to load json config");
  }
  configFile.close();
}

void config_load()
{
  if (SPIFFS.begin())
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        load_from_file(configFile);
      }
    }
    SPIFFS.end();
  }
  else
  {
    Serial.println("failed to mount FS");
  }
}

void save_config()
{
  char no_of_leds[4]; // max uint8
  char mqtt_port[6];  // max uint16

  itoa(config.no_of_leds, no_of_leds, 10);
  itoa(config.mqtt_port, mqtt_port, 10);

  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["mqtt_server"] = config.mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_username"] = config.mqtt_username;
  json["mqtt_password"] = config.mqtt_password;
  json["no_of_leds"] = no_of_leds;

  SPIFFS.begin();
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
    SPIFFS.end();
    return;
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
  //end save
  SPIFFS.end();
}
