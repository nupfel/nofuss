<?php

use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Output\OutputInterface;

class GitHubImport extends Command {

   private $settings;

   public function __construct($settings) {
      $this->settings = $settings['settings'];
      parent::__construct();
   }


   protected function configure() {

      $this
         ->setName('nofuss:github')
         ->setDescription('Import latest release from GitHub')
      ;

   }

   protected function execute(InputInterface $input, OutputInterface $output) {

      $client = new \Github\Client();
      $api = $client->api('repo')->releases();
      $release = $api->latest('xoseperez', 'espurna');
      $version = $release["tag_name"];
      $date = $release["published_at"];

      $filename = $this->settings['database']['path'];
      if (is_file($filename) && !is_writable($filename)) {
          $output->writeln("ERROR database file is not writable...");
          return;
      }
      if (!($handle = fopen($filename, 'w'))) {
          $output->writeln("ERROR opening database file...");
          return;
      }
  
      $binaries = $this->settings['binaries']['path'] . "/$version";
      if (!is_dir($binaries)) {
          if (!mkdir($binaries)) {
              $output->writeln("ERROR creating binaries destination folder...");
              return;
          }
      }

      $error = false;
      $first = true;
      fwrite($handle, "[");
      foreach ($release["assets"] as $asset) {
  
         $device = str_replace("espurna-$version-", "", $asset["name"]);
         $device = str_replace(".bin", "", $device);
         $device = str_replace("-", "_", $device);
         $device = strtoupper($device);

         $output->write($device . " -> " . $asset["name"]);

         $binary = $binaries . "/" . $asset["name"];
         if (!is_file($binary)) {
            if (!copy($asset["browser_download_url"], $binary)) {
               $output->writeln("  ERROR downloading file...");
               $error = true;
               continue;
            }
         }

         if ($first) {
            $first = false;
         } else {
            fwrite($handle, ",");
         }
         fwrite($handle, "\n  {\n");
         fwrite($handle, "    \"origin\": {\"device\": \"$device\", \"lt\": \"$version\"},\n");
         fwrite($handle, "    \"target\": {\"device\": \"$device\", \"version\": \"$version\", \"firmware\": \"/firmware/$version/" . $asset["name"] . "\", \"size\": " . $asset["size"] . "}\n");
         fwrite($handle, "  }");
         $output->writeln("  OK");
  
      }
      fwrite($handle, "\n]");
  
      if ($error) {
          $output->writeln("There have been some errors!!");
      } else {
          $output->writeln("OK");
      }
      return;
      

   }

}
