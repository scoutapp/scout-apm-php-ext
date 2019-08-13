--TEST--
Calls to file_get_contents are logged
--FILE--
<?php
file_get_contents(__FILE__);
?>
--EXPECTF--
Entered @ %f: file_get_contents
Exited  @ %f: file_get_contents
