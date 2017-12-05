<?php
if (PHP_SAPI == 'cli-server') {
    // To help the built-in PHP dev server, check if the request was actually for
    // something which should probably be served as a static file
    $url  = parse_url($_SERVER['REQUEST_URI']);
    $file = __DIR__ . $url['path'];
    if (is_file($file)) {
        return false;
    }
}

// Define the absolute path to the project root (where /vendor and /src folders are)
$root = __DIR__ . '/..';
//$root = __DIR__ . '/../../nofuss';

require $root . '/vendor/autoload.php';

session_start();

// Instantiate the app
$settings = require $root . '/src/settings.php';
$app = new \Slim\App($settings);

// Set up dependencies
require $root . '/src/dependencies.php';

// Register middleware
require $root . '/src/middleware.php';

// Register routes
require $root . '/src/routes.php';

// Run app
$app->run();
