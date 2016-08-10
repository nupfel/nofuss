# NoFUSS

NoFUSS is a **Firmware Update Server for ESP8266** modules. It defines a protocol and its implementation as a PHP service and a C++ client library so your devices can check for updates, download them and flash them autonomously.

## The protocol

This first revision of the protocol is very simple. The client device does a GET request to a custom URL specifying its DEVICE and firmware VERSION this way:

```
GET http://myNofussServerURL/DEVICE/VERSION
```

For instance:

```
GET http://192.168.1.10/nofuss/SONOFF/0.1.0
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

The PHP server implementation depends on [Slim Framework][3], [Monolog][4] and [Akrabat IP Address Middleware][5]. They are all set as dependencies in the composer.json file, so you just have to type `php composer.phar install` from the server folder.

Next you will have to configure your webserver to configure the URLs. If you are using [Apache][6] then all you have to do is create a new service pointing to the ```server/public``` folder. The ```.htaccess``` file there will take care of the rest. If you are using [Nginx][7] the create a new site file like this one:

```
server {
	listen 80 default_server;
	server_name nofuss.local;
	root /<path_to_project>/server/public/;
	try_files $uri $uri/ /index.php?$query_string;
	index index.php;
	include global/php5-fpm.conf;
}
```

Make sure the server has permissions to write on the ```logs``` folder.

## Versions

The versions info is stored in the ```data/versions.json``` file. This file contains an array of objects with info about version matching and firmware files. Version matching is always "more or equal" for minimum version number and "less or equal" for maximum version number. An asterisk (\*) means "any". Device matching is "equals".

The target key contains info about version number for the new firmware and paths to the firmware files relative to the ```public``` folder. If there is no binary for "firmware" or "spiffs" keys, just leave it empty.

```
[
    {
        "origin": {
            "device": "TEST",
            "min": "*",
            "max": "0.1.0"
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

## Notes

The library uses official ESP8266httpUpdate library. Current version of the library restarts the modules after SPIFFS update, thus preventing the firmware to be updated too. There is a recent commit fixing that which is not yet pushed to PlatformIO. Check [Fix example for ESP8266httpUpdate][5] for more info.

With the new version the library will work fine, but you will never get the final NOFUSS_FIRMWARE_UPDATED message since the board will reset before. To fix this modify the ESP8266httpUpdate.cpp file to comment out the ```ESP.restart()``` line. The library will then emit the message and restart the board itself.

```
299,300c299,300
<                     if(_rebootOnUpdate) {
<                         ESP.restart();
---
>                     if(_rebootOnUpdate) {
>                         //ESP.restart();
```



[1]: https://github.com/bblanchon/ArduinoJson
[2]: https://platformio.org
[3]: http://www.slimframework.com/
[4]: https://github.com/Seldaek/monolog
[5]: https://github.com/akrabat/rka-ip-address-middleware
[6]: https://httpd.apache.org/
[7]: https://nginx.org/
