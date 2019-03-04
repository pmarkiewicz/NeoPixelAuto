#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h>

#include "config.h"

config_struct config = {"", "1883", "", "", 8};

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
    strlcpy(config.mqtt_port, json["mqtt_port"] | "", sizeof(config.mqtt_port));
    strlcpy(config.mqtt_username, json["mqtt_username"] | "", sizeof(config.mqtt_username));
    strlcpy(config.mqtt_password, json["mqtt_password"] | "", sizeof(config.mqtt_password));
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
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["mqtt_server"] = config.mqtt_server;
  json["mqtt_port"] = config.mqtt_port;
  json["mqtt_username"] = config.mqtt_username;
  json["mqtt_password"] = config.mqtt_password;

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
