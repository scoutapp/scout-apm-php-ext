--TEST--
Bug https://github.com/scoutapp/scout-apm-php-ext/issues/47 - fix segfault when accessing argument store out of bounds
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php
scoutapm_enable_instrumentation(true);

$f1 = fopen(tempnam(sys_get_temp_dir(), 'scoutapm-test'), 'w+');
$f2 = tmpfile();

fwrite($f2, "fread/fwrite test");
var_dump(scoutapm_get_calls()[0]['argv']);
?>
--EXPECTF--
array(2) {
  [0]=>
  string(%d) "resource(%d)"
  [1]=>
  string(%d) "fread/fwrite test"
}
