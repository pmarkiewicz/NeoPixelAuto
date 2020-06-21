#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "ntp.h"
#include "dst.h"
#include "ntp_time.h"

static WiFiUDP UDP;                     // Create an instance of the WiFiUDP class to send and receive

unsigned long previousTime = millis();

const unsigned long intervalNTPTimeout = 30000;
static unsigned long intervalNTPUpdate;
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = 0;
unsigned long lastNTPSend = 0;

enum NTPState {
  not_started,
  started
};

NTPState ntp_state = not_started;
uint32_t time_unix = 0;

static void updateNTP(uint32_t currentMillis) {
    //Serial.println("NTP await");
    uint32_t ntpTime = getTime(UDP);
    if (ntpTime) {
      Serial.print("NTP recived: ");
      Serial.println(time_unix);
      ntp_state = not_started;
      lastNTPResponse = currentMillis;
      time_unix = ntpTime;
    }
    else if (currentMillis - lastNTPSend > intervalNTPTimeout) {
      Serial.println("NTP not received");
      ntp_state = not_started;
    }
}

static void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(UDP_PORT);                          // Start listening for UDP messages
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

static void startNTP(uint32_t currentMillis) {
    Serial.println("Sending NTP");
    if (sendNTPpacket(UDP)) {
      Serial.println("NTP sent");
      ntp_state = started;
      lastNTPSend = currentMillis;
    }
}

void ntp_time_init(unsigned long update_interval) {
    startUDP();
    intervalNTPUpdate = update_interval;

    startUDP();
}

void ntp_time_update() {
  unsigned long currentMillis = millis();

  // start time update
  uint32_t dt = currentMillis - lastNTPResponse;
  if (ntp_state == not_started && (time_unix == 0 || dt > intervalNTPUpdate) ) {
    startNTP(currentMillis);

    return;
  }

  // wait for response
  if (ntp_state == started) {
      updateNTP(currentMillis);
  }
}

void get_time(unsigned char& hr, unsigned char& min) {
    if (time_unix == 0) {
        hr = 0;
        min = 0;

        return;
    }

    uint32_t current_time = (millis() - lastNTPResponse) / 1000 + time_unix;

    struct tm *tm = gmtime((time_t*)&current_time);
    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;
    int d = tm->tm_mday;
    unsigned long tz = adjustDstEurope(y, m, d);

    unsigned long lt = current_time + tz;
    tm = gmtime((time_t*)&lt);

    hr = tm->tm_hour;
    min = tm->tm_min;
}



