--TEST--
Check Scout APM extension is loaded
--FILE--
<?php
var_dump(extension_loaded('scoutapm'));
?>
--EXPECT--
bool(true)
