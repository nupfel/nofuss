/*

   NOFUSS Client 0.4.0
   Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>
   Copyright (C) 2020 by Twoflower Tourist <tobi at kitten dot nz>

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

#include "NoFUSSClient.h"

BearSSL::CertStore certStore;

void NoFUSSClientClass::_initCertStore() {
    _disabled = false;

    if (_server.startsWith("https")) {
        LittleFS.begin();
        int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));

#ifdef DEBUG_NOFUSS
        Serial.print(F("Number of CA certs read: "));
        Serial.println(numCerts);
#endif

        if (numCerts == 0) {
#ifdef DEBUG_NOFUSS
            Serial.println(F("No certs found. Did you run certs-from-mozill.py and upload the LittleFS directory before running?"));
#endif
            _disabled = true;
            LittleFS.end();
            return; // Can't connect to anything w/o certs!
        }
    }
    else {
        LittleFS.end();
    }
}

void NoFUSSClientClass::setServer(String server) {
    _server = server;
    _initCertStore();
}

void NoFUSSClientClass::setDevice(String device) {
    _device = device;
}

void NoFUSSClientClass::setVersion(String version) {
    _version = version;
}

void NoFUSSClientClass::setBuild(String build) {
    _build = build;
}

void NoFUSSClientClass::onMessage(TMessageFunction fn) {
    _callback = fn;
}

String NoFUSSClientClass::getNewVersion() {
    return _newVersion;
}

String NoFUSSClientClass::getNewFirmware() {
    return _newFirmware;
}

String NoFUSSClientClass::getNewFileSystem() {
    return _newFileSystem;
}

int NoFUSSClientClass::getErrorNumber() {
    return _errorNumber;
}

String NoFUSSClientClass::getErrorString() {
    return _errorString;
}

void NoFUSSClientClass::_doCallback(nofuss_t message) {
    if (_callback != NULL) _callback(message);
}

String NoFUSSClientClass::_getPayload() {

    String payload = "";
    HTTPClient http;

    if (_server.startsWith("https://")) {
        _setClock();

        BearSSL::WiFiClientSecure *ssl_client = new BearSSL::WiFiClientSecure();

        bool mfln = ssl_client->probeMaxFragmentLength(_extractDomain(_server), 443, 1024); // domain must be the same as in ESPhttpUpdate.update()
        Serial.printf("MFLN supported: %s\n", mfln ? "yes" : "no");
        if (mfln) {
            ssl_client->setBufferSizes(1024, 1024);
        }
        ssl_client->setCertStore(&certStore);
        http.begin(dynamic_cast<WiFiClient&>(*ssl_client), _server.c_str());
    }
    else {
#ifdef HTTPUPDATE_1_2_COMPATIBLE
        http.begin(_server.c_str());
#else
        WiFiClient client;
        http.begin(client, _server.c_str());
#endif
    }

    http.useHTTP10(true);
    http.setReuse(false);
    http.setTimeout(HTTP_TIMEOUT);
    http.setUserAgent(F(HTTP_USERAGENT));
    http.addHeader(F("X-ESP8266-MAC"), WiFi.macAddress());
    http.addHeader(F("X-ESP8266-DEVICE"), _device);
    http.addHeader(F("X-ESP8266-VERSION"), _version);
    http.addHeader(F("X-ESP8266-BUILD"), _build);
    http.addHeader(F("X-ESP8266-CHIPID"), String(ESP.getChipId()));
    http.addHeader(F("X-ESP8266-CHIPSIZE"), String(ESP.getFlashChipRealSize()));
    http.addHeader(F("X-ESP8266-AP-MAC"), WiFi.softAPmacAddress());
    http.addHeader(F("X-ESP8266-FREE-SPACE"), String(ESP.getFreeSketchSpace()));
    http.addHeader(F("X-ESP8266-SKETCH-SIZE"), String(ESP.getSketchSize()));
    http.addHeader(F("X-ESP8266-SKETCH-MD5"), String(ESP.getSketchMD5()));
    http.addHeader(F("X-ESP8266-SDK-VERSION"), ESP.getSdkVersion());

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
    } else {
        _errorNumber = httpCode;
        _errorString = http.errorToString(httpCode);
    }
    http.end();

    return payload;

}

bool NoFUSSClientClass::_checkUpdates() {

    String payload = _getPayload();
    if (payload.length() == 0) {
        _doCallback(NOFUSS_NO_RESPONSE_ERROR);
        return false;
    }

    StaticJsonDocument<500> jsonDoc;
    auto error = deserializeJson(jsonDoc, payload);

    if (error) {
#ifdef DEBUG_NOFUSS
        Serial.printf("deserializeJson() failed with code %s\n", error.c_str());
#endif
        _doCallback(NOFUSS_PARSE_ERROR);
        return false;
    }

    if (jsonDoc.size() == 0) {
        _doCallback(NOFUSS_UPTODATE);
        return false;
    }

    _newVersion = jsonDoc["version"].as<String>();
    _newFileSystem = jsonDoc["spiffs"].as<String>();
    _newFirmware = jsonDoc["firmware"].as<String>();

    if (_newVersion == _version) {
        _doCallback(NOFUSS_UPTODATE);
        return false;
    }

    _doCallback(NOFUSS_UPDATING);
    return true;

}

void NoFUSSClientClass::_doUpdate() {

    String url;
    bool error = false;
    uint8_t updates = 0;
    t_httpUpdate_return ret;

    ESPhttpUpdate.rebootOnUpdate(false);

    if (_newFileSystem.length() > 0) {

        // Update LittleFS
        if (_newFileSystem.startsWith("http")) {
            url = _newFileSystem;
        }
        else {
            url = _server + String("/") + _newFileSystem;
        }

        if (url.startsWith("https://")) {
            BearSSL::WiFiClientSecure ssl_client = _createSSLClient(_extractDomain(url));
            ret = ESPhttpUpdate.updateFS(ssl_client, url);
        }
        else {
#ifdef HTTPUPDATE_1_2_COMPATIBLE
            ret = ESPhttpUpdate.updateSpiffs(url);
#else
            WiFiClient client;
            ret = ESPhttpUpdate.updateFS(client, url);
#endif
        }

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = ESPhttpUpdate.getLastError();
            _errorString = ESPhttpUpdate.getLastErrorString();
            _doCallback(NOFUSS_FILESYSTEM_UPDATE_ERROR);
        }
        else if (ret == HTTP_UPDATE_OK) {
            // re-initialise LittleFS and certStore
            LittleFS.end();
            _initCertStore();

            updates++;
            _doCallback(NOFUSS_FILESYSTEM_UPDATED);
        }

    }

    if (!error && (_newFirmware.length() > 0)) {

        // Update firmware
        if (_newFirmware.startsWith("http")) {
            url = _newFirmware;
        }
        else {
            url = _server + String("/") + _newFirmware;
        }

        if (url.startsWith("https://")) {
            BearSSL::WiFiClientSecure ssl_client = _createSSLClient(_extractDomain(url));
            ret = ESPhttpUpdate.update(ssl_client, url);
        }
        else {
#ifdef HTTPUPDATE_1_2_COMPATIBLE
            t_httpUpdate_return ret = ESPhttpUpdate.update(url);
#else
            WiFiClient client;
            t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
#endif
        }

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = ESPhttpUpdate.getLastError();
            _errorString = ESPhttpUpdate.getLastErrorString();
            _doCallback(NOFUSS_FIRMWARE_UPDATE_ERROR);
        } else if (ret == HTTP_UPDATE_OK) {
            updates++;
            _doCallback(NOFUSS_FIRMWARE_UPDATED);
        }

    }

    if (!error && (updates > 0)) {
        _doCallback(NOFUSS_RESET);
        ESP.restart();
    }

}

// Set time via NTP, as required for x.509 validation
void NoFUSSClientClass::_setClock() {
    configTime(0, 0, "pool.ntp.org"); // UTC

#ifdef DEBUG_NOFUSS
    Serial.print(F("Waiting for NTP time sync: "));
#endif
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        yield();
        delay(500);
#ifdef DEBUG_NOFUSS
        Serial.print(F("."));
#endif
        now = time(nullptr);
    }

#ifdef DEBUG_NOFUSS
    Serial.println();
#endif

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

#ifdef DEBUG_NOFUSS
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeinfo));
#endif
}

String NoFUSSClientClass::_extractDomain(String url) {
    url.remove(0, 8); // remove https://
    int index = url.indexOf("/");
    String domain = url.substring(0, index);
    index = domain.indexOf("@");
    if (index >= 0)
        domain.remove(0, index + 1); // remove auth part including @
    index = domain.indexOf(":");
    if (index >= 0)
        domain.remove(0, index + 1); // remove :port

    return domain;
}

BearSSL::WiFiClientSecure NoFUSSClientClass::_createSSLClient(String host) {
    BearSSL::WiFiClientSecure ssl_client;

    bool mfln = ssl_client.probeMaxFragmentLength(host, 443, 1024);
    Serial.printf("MFLN supported: %s\n", mfln ? "yes" : "no");
    if (mfln) {
        ssl_client.setBufferSizes(1024, 1024);
    }
    ssl_client.setCertStore(&certStore);

    return ssl_client;
}


void NoFUSSClientClass::handle() {

    if (_disabled) return;

    if (WiFi.status() != WL_CONNECTED) return;

    static unsigned long last_check = 0;
    if ((last_check > 0) && ((millis() - last_check) < NOFUSS_INTERVAL)) return;
    last_check = millis();

    _doCallback(NOFUSS_START);
    if (_checkUpdates()) _doUpdate();
    _doCallback(NOFUSS_END);
}

NoFUSSClientClass NoFUSSClient;
