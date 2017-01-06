/*

NOFUSS Client Basic Example
Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "NoFUSSClient.h"

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

#define WIFI_SSID               "..."
#define WIFI_PASS               "..."

#define DEVICE                  "TEST"
#define VERSION                 "0.1.0"

// Check that your server actually returns something in this URL
// In particular, remember to add a trailing slash for root requests
#define NOFUSS_SERVER           "http://192.168.1.100/"
#define NOFUSS_INTERVAL         10000

#define WIFI_CONNECT_TIMEOUT    5000
#define WIFI_RECONNECT_DELAY    5000

// -----------------------------------------------------------------------------
// NoFUSS Management
// -----------------------------------------------------------------------------

void nofussSetup() {

    NoFUSSClient.setServer(NOFUSS_SERVER);
    NoFUSSClient.setDevice(DEVICE);
    NoFUSSClient.setVersion(VERSION);

    NoFUSSClient.onMessage([](nofuss_t code) {

        if (code == NOFUSS_START) {
            Serial.println(F("[NoFUSS] Start"));
        }

        if (code == NOFUSS_UPTODATE) {
            Serial.println(F("[NoFUSS] Nothing for me"));
        }

        if (code == NOFUSS_PARSE_ERROR) {
            Serial.println(F("[NoFUSS] Error parsing server response"));
        }

        if (code == NOFUSS_UPDATING) {
            Serial.println(F("[NoFUSS] Updating"));
            Serial.print(  F("         New version: "));
            Serial.println(NoFUSSClient.getNewVersion());
            Serial.print(  F("         Firmware: "));
            Serial.println(NoFUSSClient.getNewFirmware());
            Serial.print(  F("         File System: "));
            Serial.println(NoFUSSClient.getNewFileSystem());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
            Serial.print(F("[NoFUSS] File System Update Error: "));
            Serial.println(NoFUSSClient.getErrorString());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
            Serial.println(F("[NoFUSS] File System Updated"));
        }

        if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
            Serial.print(F("[NoFUSS] Firmware Update Error: "));
            Serial.println(NoFUSSClient.getErrorString());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
            Serial.println(F("[NoFUSS] Firmware Updated"));
        }

        if (code == NOFUSS_RESET) {
            Serial.println(F("[NoFUSS] Resetting board"));
        }

        if (code == NOFUSS_END) {
            Serial.println(F("[NoFUSS] End"));
        }

    });

}

void nofussLoop() {
    static unsigned long last_check = 0;
    if (WiFi.status() != WL_CONNECTED) return;
    if ((last_check > 0) && ((millis() - last_check) < NOFUSS_INTERVAL)) return;
    last_check = millis();
    NoFUSSClient.handle();
}

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print(F("[WIFI] Connecting to "));
    Serial.println(WIFI_SSID);

    // Wait
    unsigned long timeout = millis() + WIFI_CONNECT_TIMEOUT;
    while (timeout > millis()) {
        if (WiFi.status() == WL_CONNECTED) break;
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {

        // Connected!
        Serial.print(F("[WIFI] STATION Mode, SSID: "));
        Serial.print(WiFi.SSID());
        Serial.print(F(", IP address: "));
        Serial.println(WiFi.localIP());

        // Check for updates!
        NoFUSSClient.handle();

    } else {

        Serial.println(F("[WIFI] Not connected"));

    }

}

void wifiLoop() {

    static unsigned long last_check = 0;

    // Check disconnection
    if (WiFi.status() != WL_CONNECTED) {
        if ((millis() - last_check) > WIFI_RECONNECT_DELAY) {
            wifiSetup();
            last_check = millis();
        }
    }

}

// -----------------------------------------------------------------------------
// Main methods
// -----------------------------------------------------------------------------

void setup() {

    Serial.begin(115200);

    delay(5000);
    Serial.println();
    Serial.println();
    Serial.print("Device : ");
    Serial.println(DEVICE);
    Serial.print("Version: ");
    Serial.println(VERSION);

    nofussSetup();
}

void loop() {
    wifiLoop();
    nofussLoop();
    delay(1);
}
