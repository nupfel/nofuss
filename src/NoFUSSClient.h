/*

NOFUSS Client 0.2.3
Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

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

class NoFUSSClientClass {

  public:

    typedef std::function<void(nofuss_t)> TMessageFunction;

    void setServer(String server);
    void setDevice(String device);
    void setVersion(String version);

    String getNewVersion();
    String getNewFirmware();
    String getNewFileSystem();

    int getErrorNumber();
    String getErrorString();

    void onMessage(TMessageFunction fn);
    void handle();

  private:

    String _server;
    String _device;
    String _version;

    String _newVersion;
    String _newFirmware;
    String _newFileSystem;

    int _errorNumber;
    String _errorString;

    TMessageFunction _callback = NULL;

    String _getPayload();
    bool _checkUpdates();
    void _doUpdate();
    void _doCallback(nofuss_t message);

};

extern NoFUSSClientClass NoFUSSClient;

#endif /* _NOFUSS_h */
