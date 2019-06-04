<?php

function get(&$var, $default = null) {
    return isset($var) ? $var : $default;
}

/**
 * Returns TRUE if it matches
 */
function compare($entry, $key, $operand, $version) {
    $value = get($entry['origin'][$key], "*");
    if ($value != '*') {
        return (version_compare($value, $version, $operand));
    }
    return true;
}

// Check update entry point
$app->get('/', function($request, $response, $args) {

    $found = false;
    $headers = $request->getHeaders();
    //$this->get('devices')->info(serialize($headers));
    $mac = $headers['HTTP_X_ESP8266_MAC'][0];
    $device = $headers['HTTP_X_ESP8266_DEVICE'][0];
    $version = $headers['HTTP_X_ESP8266_VERSION'][0];
    $build = $headers['HTTP_X_ESP8266_BUILD'][0];

    if (($mac != "") & ($device != "") & ($version != "") ) {

        foreach ($this->get('data') as $entry) {

            // Check same device
            $_device = get($entry['origin']['device'], "");
            if ($device != $_device) continue;

            // Check if MAC is same or in array
            $_mac = get($entry['origin']['mac'], "*");
            if ($_mac != '*') {
                if (is_array($_mac)) {
                    if (!in_array($mac, $_mac)) continue;
                } else {
                    if ($mac != $_mac) continue;
                }
            }

            // Version checks (gt==min, ge, lt==max, le, eq)
            if (!compare($entry, 'gt', '<', $version)) continue;
            if (!compare($entry, 'ge', '<=', $version)) continue;
            if (!compare($entry, 'min', '<=', $version)) continue;
            if (!compare($entry, 'lt', '>', $version)) continue;
            if (!compare($entry, 'max', '>', $version)) continue;
            if (!compare($entry, 'le', '>=', $version)) continue;
            if (!compare($entry, 'eq', '==', $version)) continue;

            // If build_not is defined in the JSON file, then the request must also have defined the build and they must NOT match
            $_build = get($entry['origin']['build_not'], "*");
            if ($_build != "*") {
                if ($build == "") continue;
                if ($build == $_build) continue;
            }

            // Add SPIFFS key if there is none, required for back-compatibility
            if (!in_array("spiffs", $entry)) {
                $entry["spiffs"] = "";
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
            . "BUILD: " . $build . " "
            . "UPDATE: " . ($found ? $entry['target']['version'] : "none")
        );

    }

    if (!$found) {
        $response->getBody()->write("{}");
    }

    return $response;

});

