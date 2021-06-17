--TEST--
Calls to fwrite and fread are logged with handle from fopen()
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php

var_dump(in_array('fread', scoutapm_list_instrumented_functions()));
var_dump(in_array('fwrite', scoutapm_list_instrumented_functions()));
scoutapm_enable_instrumentation(true);

$fh = fopen(tempnam(sys_get_temp_dir(), 'scoutapm-test'), 'w+');

fwrite($fh, "fread/fwrite test\n");
fseek($fh, 0);
echo fread($fh, 18);
fclose($fh);

$calls = scoutapm_get_calls();

var_dump($calls[0]['function']);
var_dump($calls[0]['argv']);

var_dump($calls[1]['function']);
var_dump($calls[1]['argv']);
?>
--EXPECTF--
bool(true)
bool(true)
fread/fwrite test
string(6) "fwrite"
array(2) {
  [0]=>
  string(%d) "/tmp/scoutapm-test%s"
  [1]=>
  string(%d) "w+"
}
string(5) "fread"
array(2) {
  [0]=>
  string(%d) "/tmp/scoutapm-test%s"
  [1]=>
  string(%d) "w+"
}
