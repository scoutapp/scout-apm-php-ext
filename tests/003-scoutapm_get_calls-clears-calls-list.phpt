--TEST--
Ensures that calls to scoutapm_get_calls() clears the call list
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
--FILE--
<?php
file_get_contents(__FILE__);
var_dump(count(scoutapm_get_calls()));
var_dump(count(scoutapm_get_calls()));
file_get_contents(__FILE__);
var_dump(count(scoutapm_get_calls()));
var_dump(count(scoutapm_get_calls()));
?>
--EXPECT--
int(1)
int(0)
int(1)
int(0)
