--TEST--
Calls to file_get_contents are logged
--FILE--
<?php
file_get_contents(__FILE__);
// @todo also expect "Exited: file_get_contents" - https://github.com/scoutapp/scout-apm-php-ext/issues/3
?>
--EXPECT--
Entered: file_get_contents
