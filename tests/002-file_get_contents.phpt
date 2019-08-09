--TEST--
Calls to file_get_contents are logged
--FILE--
<?php
file_get_contents(__FILE__);
?>
--EXPECT--
Observed: file_get_contents
