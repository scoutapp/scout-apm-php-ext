--TEST--
When calls to functions are recorded, and scoutapm_get_calls is not called, don't leak memory
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php if (!defined("PHP_DEBUG") || PHP_DEBUG != 1) die("skip Recompile PHP with --enable-debug."); ?>
--FILE--
<?php
file_put_contents(__FILE__ . ".x", file_get_contents(__FILE__));
unlink(__FILE__ . ".x");
echo "end";
?>
--EXPECTREGEX--
end((?!memory leaks detected).)*
