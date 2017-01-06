<?php

function get(&$var, $default = null) {
    return isset($var) ? $var : $default;
}

// Check update entry point
$app->get('/', function($request, $response, $args) {

    $found = false;
    $headers = $request->getHeaders();
    //$this->get('devices')->info(serialize($headers));
    $mac = $headers['HTTP_X_ESP8266_MAC'][0];
    $device = $headers['HTTP_X_ESP8266_DEVICE'][0];
    $version = $headers['HTTP_X_ESP8266_VERSION'][0];

    if ($mac & $device & $version ) {

        foreach ($this->get('data') as $entry) {

            if ($_device = get($entry['origin']['device'])) {
                if ($device != $_device) continue;
            }

            if ($_mac = get($entry['origin']['mac'], "*")) {
                if (($_mac != '*') && ($mac != $_mac)) continue;
            }

            if ($_min = get($entry['origin']['min'], "*")) {
                if (($_min != '*') && version_compare($_min, $version, '>')) continue;
            }

            if ($_max = get($entry['origin']['max'], "*")) {
                if (($_max != '*') && version_compare($_max, $version, '<')) continue;
            }

            $response->getBody()->write(stripslashes(json_encode($entry['target'])));
            $found = true;
            break;

        };

        $this->get('devices')->info(
            "IP: " . $request->getAttribute('ip_address') . " "
            . "MAC: " . $mac . " "
            . "DEVICE: " . $device . " "
            . "VERSION: " . $version . " "
            . "UPDATE: " . ($found ? $entry['target']['version'] : "none")
        );

    }

    if (!$found) {
        $response->getBody()->write("{}");
    }

    return $response;

});
