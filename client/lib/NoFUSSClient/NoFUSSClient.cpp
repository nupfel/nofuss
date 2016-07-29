/*

NOFUSS Client
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

#include "NoFUSSClient.h"
#include <functional>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

void NoFUSSClientClass::setServer(String server) {
    _server = server;
}

void NoFUSSClientClass::setDevice(String device) {
    _device = device;
}

void NoFUSSClientClass::setVersion(String version) {
    _version = version;
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

String NoFUSSClientClass::_getPayload() {

    HTTPClient http;
    char url[100];
    String payload = "";

    sprintf(url, "%s/%s/%s", _server.c_str(), _device.c_str(), _version.c_str());
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) payload = http.getString();
    http.end();

    return payload;

}

bool NoFUSSClientClass::_checkUpdates() {

    String payload = _getPayload();
    if (payload.length() == 0) {
        _callback(NOFUSS_NO_RESPONSE_ERROR);
        return false;
    }

    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& response = jsonBuffer.parseObject(payload);

    if (!response.success()) {
        _callback(NOFUSS_PARSE_ERROR);
        return false;
    }

    if (response.size() == 0) {
        _callback(NOFUSS_UPTODATE);
        return false;
    }

    _newVersion = response.get<String>("version");
    _newFileSystem = response.get<String>("spiffs");
    _newFirmware = response.get<String>("firmware");

    _callback(NOFUSS_UPDATING);
    return true;

}

void NoFUSSClientClass::_doUpdate() {

    char url[100];
    bool error = false;
    uint8_t updates = 0;

    if (_newFileSystem.length() > 0) {

        // Update SPIFFS
        sprintf(url, "%s/%s", _server.c_str(), _newFileSystem.c_str());
        t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url);

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = ESPhttpUpdate.getLastError();
            _errorString = ESPhttpUpdate.getLastErrorString();
            _callback(NOFUSS_FILESYSTEM_UPDATE_ERROR);
        } else if (ret == HTTP_UPDATE_OK) {
            updates++;
            _callback(NOFUSS_FILESYSTEM_UPDATED);
        }

    }

    if (!error && (_newFirmware.length() > 0)) {

        // Update binary
        sprintf(url, "%s%s", _server.c_str(), _newFirmware.c_str());
        t_httpUpdate_return ret = ESPhttpUpdate.update(url);

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = ESPhttpUpdate.getLastError();
            _errorString = ESPhttpUpdate.getLastErrorString();
            _callback(NOFUSS_FIRMWARE_UPDATE_ERROR);
        } else if (ret == HTTP_UPDATE_OK) {
            updates++;
            _callback(NOFUSS_FIRMWARE_UPDATED);
        }

    }

    if (!error && (updates > 0)) {
        _callback(NOFUSS_RESET);
        ESP.restart();
    }

}

void NoFUSSClientClass::handle() {
    _callback(NOFUSS_START);
    if (_checkUpdates()) _doUpdate();
    _callback(NOFUSS_END);
}

NoFUSSClientClass NoFUSSClient;
