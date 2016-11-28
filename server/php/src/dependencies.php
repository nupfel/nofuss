<?php
// DIC configuration

$container = $app->getContainer();

// monolog
$container['devices'] = function ($container) {

    $settings = $container->get('settings')['devices'];

    $dateFormat = "[Y/m/d H:i:s]";
    $output = "%datetime% %message%\n";
    $formatter = new Monolog\Formatter\LineFormatter($output, $dateFormat);
    $stream = new Monolog\Handler\StreamHandler($settings['path'], Monolog\Logger::INFO);
    $stream->setFormatter($formatter);

    $logger = new Monolog\Logger($settings['name']);
    $logger->pushHandler($stream);

    return $logger;

};

// version database
$container['data'] = function($container) {
    $settings = $container->get('settings')['database'];
    $json_data = file_get_contents($settings['path']);
    $data = json_decode($json_data, true);
    return $data;
};
