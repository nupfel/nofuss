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

#ifndef _NOFUSS_h
#define _NOFUSS_h

#include <functional>
#include <Stream.h>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

#include <time.h>

#include <FS.h>
#include <LittleFS.h>

// A single, global CertStore which can be used by all
// connections.  Needs to stay live the entire time any of
// the WiFiClientBearSSLs are present.
#include <CertStoreBearSSL.h>

typedef enum {
    NOFUSS_START,
    NOFUSS_UPTODATE,
    NOFUSS_UPDATING,
    NOFUSS_FILESYSTEM_UPDATED,
    NOFUSS_FIRMWARE_UPDATED,
    NOFUSS_RESET,
    NOFUSS_END,
    NOFUSS_NO_RESPONSE_ERROR,
    NOFUSS_PARSE_ERROR,
    NOFUSS_FILESYSTEM_UPDATE_ERROR,
    NOFUSS_FIRMWARE_UPDATE_ERROR
} nofuss_t;

#ifndef NOFUSS_INTERVAL
#define NOFUSS_INTERVAL 1000 * 60 * 10 // 10 min.
#endif
#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif
#define HTTP_TIMEOUT    10000
#define HTTP_USERAGENT  "NoFussClient"

class NoFUSSClientClass {

public:

    NoFUSSClientClass() {
        _HttpUpdate.setLedPin(LED_BUILTIN, LOW);
    };

    NoFUSSClientClass(int httpClientTimeout) {
        _HttpUpdate = ESP8266HTTPUpdate(httpClientTimeout);
        _HttpUpdate.setLedPin(LED_BUILTIN, LOW);
    };

    typedef std::function<void (nofuss_t)> TMessageFunction;

    void setFwUrl(String fwUrl);
    void setDevice(String device);
    void setVersion(String version);
    void setBuild(String build);
    void onStart(HTTPUpdateStartCB cbOnStart);
    void onEnd(HTTPUpdateEndCB cbOnEnd);
    void onError(HTTPUpdateErrorCB cbOnError);
    void onProgress(HTTPUpdateProgressCB cbOnProgress);
    void onMessage(TMessageFunction fn);
    void handle();

    String getNewVersion();
    String getNewFirmware();
    String getNewFileSystem();

    int getErrorNumber();
    String getErrorString();

private:

    bool _disabled;

    ESP8266HTTPUpdate _HttpUpdate;

    String _fwUrl;
    String _device;
    String _version;
    String _build;

    String _newVersion;
    String _newFirmware;
    String _newFileSystem;

    int _errorNumber;
    String _errorString;

    TMessageFunction _callback = NULL;

    String _getPayload();
    String _extractDomain(String url);
    BearSSL::WiFiClientSecure _createSSLClient(String host);
    bool _checkUpdates();
    void _doUpdate();
    void _doCallback(nofuss_t message);
    void _setClock();
    void _initCertStore();

};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_NOFUSS)
extern NoFUSSClientClass NoFUSSClient;
#endif

#endif     /* _NOFUSS_h */
