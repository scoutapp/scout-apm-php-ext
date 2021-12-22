--TEST--
Both URL and Method can be captured using file_get_contents
--SKIPIF--
skip not implemented yet
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php if (!extension_loaded("curl")) die("skip curl extension required."); ?>
--FILE--
<?php

var_dump(in_array('curl_exec', scoutapm_list_instrumented_functions()));
scoutapm_enable_instrumentation(true);

$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, "https://scoutapm.com/robots.txt");
curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
curl_setopt($ch, CURLOPT_POST, 1);
curl_setopt($ch, CURLOPT_USERAGENT, 'scoutapp/scout-apm-php-ext Test Suite curl');
curl_exec($ch);

$call = scoutapm_get_calls()[0];

var_dump($call['function']);
var_dump($call['argv']);
?>
--EXPECTF--
bool(true)
string(9) "curl_exec"
array(%d) {
  [%d]=>
  string(%d) "https://scoutapm.com/robots.txt"
  [%d]=>
  string(%d) "POST"
}
