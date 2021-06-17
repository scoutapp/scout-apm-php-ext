--TEST--
Do not instrument anything by default, unless scoutapm_enable_instrumentation(true) is called
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php

var_dump(in_array('file_get_contents', scoutapm_list_instrumented_functions()));

file_get_contents(__FILE__);
var_dump(scoutapm_get_calls());

scoutapm_enable_instrumentation(true);
file_get_contents(__FILE__);
var_dump(array_column(scoutapm_get_calls(), 'function'));

scoutapm_enable_instrumentation(false);
file_get_contents(__FILE__);
var_dump(scoutapm_get_calls());
?>
--EXPECTF--
bool(true)
array(0) {
}
array(1) {
  [0]=>
  string(%s) "file_get_contents"
}
array(0) {
}
