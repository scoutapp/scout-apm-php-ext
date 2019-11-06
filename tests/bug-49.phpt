--TEST--
Bug https://github.com/scoutapp/scout-apm-php-ext/issues/49 - only record arguments for fopen if it returns a resource
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php
var_dump(fopen('/this/file/should/not/exist', 'r'));
var_dump(scoutapm_get_calls());
?>
--EXPECTF--
Warning: fopen(%s): failed to open stream: No such file or directory in %s
bool(false)
array(0) {
}
