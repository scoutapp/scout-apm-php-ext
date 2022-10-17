<?php

ob_start();
phpinfo(INFO_MODULES);
$phpinfo = array_column(array_map(
  static function ($s) {
    return explode(' => ', str_replace('scoutapm curl ', '', $s));
  },
  array_values(array_filter(
    explode("\n", ob_get_contents()),
    static function ($s) {
      return str_contains($s, 'scoutapm curl');
    }
  ))
), 1, 0);
ob_end_clean();

echo sprintf("HAVE_CURL: %s\n", $phpinfo['HAVE_CURL']);
echo sprintf("Scout CURL instrumentation enabled: %s\n", $phpinfo['functions']);
echo sprintf("curl_exec function exists: %s\n", function_exists('curl_exec') ? 'Yes' : 'No');
echo sprintf("curl_exec is in instrumented list: %s\n", in_array('curl_exec', scoutapm_list_instrumented_functions()) ? 'Yes' : 'No');

scoutapm_enable_instrumentation(true);

if (function_exists('curl_exec')) {
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, "file://" . __FILE__);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  curl_exec($ch);
}

$calls = scoutapm_get_calls();

if (! array_key_exists(0, $calls)) {
  echo "curl_exec call recorded: No\n";
  exit;
}

echo sprintf("%s call recorded: Yes\n", $calls[0]['function']);
