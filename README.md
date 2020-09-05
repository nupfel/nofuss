# NoFUSS

NoFUSS is a **Firmware Update Server for ESP8266** modules. It defines a protocol and its implementation as a PHP service and a C++ client library so your devices can check for updates, download them and flash them autonomously.

## The protocol

Current version of the protocol is simple. The client device does a GET request to a custom URL along with some headers defining:

|header|description|example|
|-|-|-|
|X-ESP8266-MAC|Device MAC address|5C:CF:7F:8B:6B:26|
|X-ESP8266-DEVICE|Device type|SENSOR|
|X-ESP8266-VERSION|Application version|0.1.0|
|X-ESP8266-BUILD|Application build|611cdf3|

The URL can be anywhere your device can reach:

```
http://192.168.1.10/nofuss
```

Here you have an example call using cURL:

```
curl -H "X-ESP8266-MAC:5C:CF:7F:8C:1E:6F" -H "X-ESP8266-VERSION:0.1.0" -H "X-ESP8266-DEVICE:TEST" http://192.168.1.10/nofuss
```

The response is a JSON object. If there are no updates available it will be
empty (that is: '{}'). Otherwise it will contain info about where to find the new firmware binaries:

```
{
    'version': '0.1.1',
    'firmware': '/firmware/sonoff-0.1.1.bin',
    'spiffs': '/firmware/sonoff-0.1.1-spiffs.bin'
}
```

Binaries URLs (for the firmware and the SPIFFS file system) are relative to the server URL, so following with the example, the device will first download the SPIFFS binary from:

```
http://192.168.1.10/nofuss/firmware/sonoff-0.1.1-spiffs.bin
```

flash it and if everything went fine it will download the firmware from:

```
http://192.168.1.10/nofuss/firmware/sonoff-0.1.1.bin
```

flash it too and then restart the board.

## Installing the server

### PHP

The PHP server implementation depends on [Slim Framework][3], [Monolog][4] and [Akrabat IP Address Middleware][5]. They are all set as dependencies in the composer.json file, so you just have to type `php composer.phar install` from the server folder.

Next you will have to configure your webserver to configure the URLs. If you are using [Apache][6] then all you have to do is create a new service pointing to the ```server/public``` folder. The ```.htaccess``` file there will take care of the rest. If you are using [Nginx][7] the create a new site file like this one:

```
server {
	listen 80 default_server;
	server_name nofuss.local;
	root /<path_to_project>/server/php/public/;
	try_files $uri $uri/ /index.php?$query_string;
	index index.php;
	include global/php5-fpm.conf;
}
```

Make sure the server has permissions to write on the ```logs``` folder.

### PHP using Docker

You can use the `docker-compose.yml` file in the root of the repository to rise an instance of a local NoFUSS server using Nginx and PHP7-FPM. Just type:

```
docker-compose up
```

The docker container will use the code under `server/php` and, in particular, the database in `server/php/data/versions.json`.

### NodeJS

User Alex Suslov ported the NoFUSS Server to NodeJS. You can check his repo [node-nofuss repo on GitHub](https://github.com/alexsuslov/node-nofuss).

### Node-red

There is a flow under `server/node-red/firmware_server.json` that implements a server using CouchDB as the database backend. You will need several external palette modulee. Please install missing palettes when importing the flow.

## Versions

The versions info is stored in the `data/versions.json` file. This file contains an array of objects with info about version matching and firmware files.

The `origin` key contains filters. The server will apply those filters to the requester info. Version matching is done using different operators:

* `ge` or `min`: Reported version should be greater or equal than the target version
* `gt`: Striclty greater than
* `eq`: Strictly equal
* `lt` or `max`: Less than target version
* `le`: Less or equal

Notice `max` and `min` are there for backwards compatibility and they are not symetric: "more or equal" for minimum version number and "less or equal" for maximum version number.

An asterisk (`*`) means "any" and it's equivalent to not specifying that key. If specified, the `device` and `mac` keys must match exactly. The `build_not` filter works slightly different. If defined and different than `*` it will match any request with non-empty `X-ESP8266-BUILD` header that's different from the defined in the rule. This is meant for different build of the same version (problably a development one?) and to avoid a loop it requires the requested to report the build explicitly.

If you define no filter (i.e. the `origin` key is empty) every request will match.

The `target` key contains info about version number for the new firmware and paths to the firmware files relative to the `public` folder. The `firmware` key must be always present. If there is no binary for "firmware" just leave it empty.

```
[
    {
        "origin": {
            "mac": "5C:CF:7F:8B:6B:26",
            "device": "TEST",
            "gt": "*",
            "lt": "0.1.1",
            "build_not": "3fe56a4"
        },
        "target": {
            "version": "0.1.1",
            "firmware": "/firmware/test-0.1.1.bin",
            "spiffs": ""
        }
    }
]
```

## Using the client

The client library depends on [Benoit Blanchon's ArduinoJson][1] library. It is set as a dependency in the platformio.ini file, so if you use  [PlatformIO][2] it will automatically download.

To use it you only have to configure the global NoFUSSClient object with proper server URL, device name and version in your setup:

```
NoFUSSClient.setServer(NOFUSS_SERVER);
NoFUSSClient.setDevice(DEVICE);
NoFUSSClient.setVersion(VERSION);
```

And then call every so often ```NoFUSSClient.handle()``` to check for updates. You can also monitor the update flow providing a callback function to the ```onMessage``` method. Check the basic.cpp example for a real usage.

[1]: https://github.com/bblanchon/ArduinoJson
[2]: https://platformio.org
[3]: http://www.slimframework.com/
[4]: https://github.com/Seldaek/monolog
[5]: https://github.com/akrabat/rka-ip-address-middleware
[6]: https://httpd.apache.org/
[7]: https://nginx.org/
