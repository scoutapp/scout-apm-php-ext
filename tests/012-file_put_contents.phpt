--TEST--
Calls to file_put_contents are logged
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php

var_dump(in_array('file_put_contents', scoutapm_list_instrumented_functions()));

$file = tempnam(sys_get_temp_dir(), 'scout-apm-php-ext-test');
file_put_contents($file, 'test content');
$call = scoutapm_get_calls()[0];

var_dump(file_get_contents($file));
var_dump($call['function']);
var_dump($call['argv']);
?>
--EXPECTF--
bool(true)
string(12) "test content"
string(17) "file_put_contents"
array(2) {
  [0]=>
  string(%d) "%sscout-apm-php-ext-test%s"
  [1]=>
  string(12) "test content"
}
