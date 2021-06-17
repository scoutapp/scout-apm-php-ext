--TEST--
Calls to file_get_contents are logged
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php

var_dump(in_array('file_get_contents', scoutapm_list_instrumented_functions()));
scoutapm_enable_instrumentation(true);

file_get_contents(__FILE__);
$call = scoutapm_get_calls()[0];

var_dump($call['function']);
var_dump($call['entered']);
var_dump($call['exited']);
var_dump($call['time_taken']);
var_dump($call['exited'] > $call['entered']);
var_dump($call['argv']);
?>
--EXPECTF--
bool(true)
string(17) "file_get_contents"
float(%f)
float(%f)
float(%f)
bool(true)
array(1) {
  [0]=>
  string(%d) "%s/tests/002-file_get_contents.php"
}
