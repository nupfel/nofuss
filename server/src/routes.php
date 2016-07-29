<?php

// Check update entry point
$app->get('/{device}/{version}', function($request, $response, $args) {

    $found = false;
    $device = $request->getAttribute('device');
    $version = $request->getAttribute('version');

    foreach ($this->get('data') as $entry) {

        if (($device == $entry['origin']['device'])
            && (($entry['origin']['min'] == '*' || version_compare($entry['origin']['min'], $version, '<=')))
            && (($entry['origin']['max'] == '*' || version_compare($entry['origin']['max'], $version, '>=')))) {

            $response->getBody()->write(stripslashes(json_encode($entry['target'])));
            $found = true;
            break;

        }
    };

    if (!$found) {
        $response->getBody()->write("{}");
    }

    $this->get('devices')->info(
        "from:"
        . $request->getAttribute('ip_address')
        . " device:$device version:$version update:"
        . ($found ? $entry['target']['version'] : "none")
    );

    return $response;

});
