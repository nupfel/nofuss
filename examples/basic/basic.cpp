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
#include "credentials.h"

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// See credentials.h (copy it from credentials.sample.h)
// for specific configuration

#define DEVICE                  "TEST"
#define VERSION                 "0.0.9"
#define BUILD                   ""
#define NOFUSS_INTERVAL         10000
#define WIFI_CONNECT_TIMEOUT    20000
#define WIFI_RECONNECT_DELAY    5000

// -----------------------------------------------------------------------------
// NoFUSS Management
// -----------------------------------------------------------------------------

void nofussSetup() {

    NoFUSSClient.setServer(NOFUSS_SERVER);
    NoFUSSClient.setDevice(DEVICE);
    NoFUSSClient.setVersion(VERSION);
    NoFUSSClient.setBuild(BUILD);
    
    NoFUSSClient.onMessage([](nofuss_t code) {

        if (code == NOFUSS_START) {
            Serial.printf("[NoFUSS] Start\n");
        }

        if (code == NOFUSS_UPTODATE) {
            Serial.printf("[NoFUSS] Nothing for me\n");
        }

        if (code == NOFUSS_PARSE_ERROR) {
            Serial.printf("[NoFUSS] Error parsing server response\n");
        }

        if (code == NOFUSS_UPDATING) {
            Serial.printf("[NoFUSS] Updating\n");
            Serial.printf("         New version: %s\n", NoFUSSClient.getNewVersion().c_str());
            Serial.printf("         Firmware: %s\n", NoFUSSClient.getNewFirmware().c_str());
            Serial.printf("         File System: %s\n", NoFUSSClient.getNewFileSystem().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
            Serial.printf("[NoFUSS] File System Update Error: %s\n", NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
            Serial.printf("[NoFUSS] File System Updated\n");
        }

        if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
            Serial.printf("[NoFUSS] Firmware Update Error: %s\n", NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
            Serial.printf("[NoFUSS] Firmware Updated\n");
        }

        if (code == NOFUSS_RESET) {
            Serial.printf("[NoFUSS] Resetting board\n");
        }

        if (code == NOFUSS_END) {
            Serial.printf("[NoFUSS] End\n");
        }

    });

}

void nofussLoop() {

    if (WiFi.status() != WL_CONNECTED) return;

    static unsigned long last_check = 0;
    if ((last_check > 0) && ((millis() - last_check) < NOFUSS_INTERVAL)) return;
    last_check = millis();

    NoFUSSClient.handle();

}

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_OFF);
    delay(5);
    WiFi.mode(WIFI_STA);

    // Connect
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);

    // Wait
    unsigned long timeout = millis() + WIFI_CONNECT_TIMEOUT;
    while (timeout > millis()) {
        if (WiFi.status() == WL_CONNECTED) break;
        delay(100);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {

        // Connected!
        Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

        // Check for updates!
        NoFUSSClient.handle();

    } else {

        Serial.printf("[WIFI] Not connected\n");

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

    delay(1000);

    Serial.begin(115200);
    //Serial.setDebugOutput(true);
    Serial.println();
    Serial.printf("[MAIN] Device: %s\n", DEVICE);
    Serial.printf("[MAIN] Version: %s\n", VERSION);

    nofussSetup();

}

void loop() {
    wifiLoop();
    nofussLoop();
    delay(1);
}
