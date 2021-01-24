/*

   NOFUSS Client 0.4.1
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

    if (_fwUrl.startsWith("https")) {
        LittleFS.begin();
        int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));

#ifdef DEBUG_NOFUSS
        Serial.print(F("[NOFUSS] Number of CA certs read: "));
        Serial.println(numCerts);
#endif

        if (numCerts == 0) {
            Serial.println(F("[NOFUSS] No certs found. Did you run certs-from-mozill.py and upload the LittleFS directory before running?"));
            _disabled = true;
            LittleFS.end();
            return; // Can't connect to anything w/o certs!
        }
    }
    else {
        LittleFS.end();
    }
}

void NoFUSSClientClass::setFwUrl(String fwUrl) {
    _fwUrl = fwUrl;
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

void NoFUSSClientClass::onStart(HTTPUpdateStartCB cbOnStart) {
    _HttpUpdate.onStart(cbOnStart);
}

void NoFUSSClientClass::onEnd(HTTPUpdateEndCB cbOnEnd) {
    _HttpUpdate.onEnd(cbOnEnd);
}

void NoFUSSClientClass::onError(HTTPUpdateErrorCB cbOnError) {
    _HttpUpdate.onError(cbOnError);
}

void NoFUSSClientClass::onProgress(HTTPUpdateProgressCB cbOnProgress) {
    _HttpUpdate.onProgress(cbOnProgress);
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

    if (_fwUrl.startsWith("https://")) {
        _setClock();

        BearSSL::WiFiClientSecure *ssl_client = new BearSSL::WiFiClientSecure();

        bool mfln = ssl_client->probeMaxFragmentLength(_extractDomain(_fwUrl), 443, 1024); // domain must be the same as in _HttpUpdate.update()
#ifdef DEBUG_NOFUSS
        Serial.printf("[NOFUSS] MFLN supported: %s\n", mfln ? "yes" : "no");
#endif
        if (mfln) {
            ssl_client->setBufferSizes(1024, 1024);
        }
        ssl_client->setCertStore(&certStore);
        http.begin(dynamic_cast<WiFiClient&>(*ssl_client), _fwUrl.c_str());
    }
    else {
#ifdef HTTPUPDATE_1_2_COMPATIBLE
        http.begin(_fwUrl.c_str());
#else
        WiFiClient client;
        http.begin(client, _fwUrl.c_str());
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
        Serial.printf("[NOFUSS] deserializeJson() failed with code %s\n", error.c_str());
#endif
        _doCallback(NOFUSS_PARSE_ERROR);
        return false;
    }

    if (jsonDoc.size() == 0) {
#ifdef DEBUG_NOFUSS
        Serial.println("[NOFUSS] Empty JSON object");
#endif
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

    _HttpUpdate.rebootOnUpdate(false);

    if (_newFileSystem.length() > 0) {

        // Update LittleFS
        if (_newFileSystem.startsWith("http")) {
            url = _newFileSystem;
        }
        else {
            url = _fwUrl + String("/") + _newFileSystem;
        }

        if (url.startsWith("https://")) {
            BearSSL::WiFiClientSecure ssl_client = _createSSLClient(_extractDomain(url));
            ret = _HttpUpdate.updateFS(ssl_client, url);
        }
        else {
#ifdef HTTPUPDATE_1_2_COMPATIBLE
            ret = _HttpUpdate.updateSpiffs(url);
#else
            WiFiClient client;
            ret = _HttpUpdate.updateFS(client, url);
#endif
        }

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = _HttpUpdate.getLastError();
            _errorString = _HttpUpdate.getLastErrorString();
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
            url = _fwUrl + String("/") + _newFirmware;
        }

        if (url.startsWith("https://")) {
            BearSSL::WiFiClientSecure ssl_client = _createSSLClient(_extractDomain(url));
            ret = _HttpUpdate.update(ssl_client, url);
        }
        else {
#ifdef HTTPUPDATE_1_2_COMPATIBLE
            t_httpUpdate_return ret = _HttpUpdate.update(url);
#else
            WiFiClient client;
            t_httpUpdate_return ret = _HttpUpdate.update(client, url);
#endif
        }

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = _HttpUpdate.getLastError();
            _errorString = _HttpUpdate.getLastErrorString();
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
    configTime(0, 0, NTP_SERVER); // UTC

#ifdef DEBUG_NOFUSS
    Serial.print(F("[NOFUSS] Waiting for NTP time sync from "));
    Serial.print(F(NTP_SERVER));
    Serial.print(F(": "));
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
    Serial.print(F("[NOFUSS] Current time: "));
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
    Serial.printf("[NOFUSS] MFLN supported: %s\n", mfln ? "yes" : "no");
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

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_NOFUSS)
NoFUSSClientClass NoFUSSClient;
#endif
