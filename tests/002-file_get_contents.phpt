--TEST--
Calls to file_get_contents are logged
--FILE--
<?php
file_get_contents(__FILE__);
// @todo scoutapm_get_calls() - https://github.com/scoutapp/scout-apm-php-ext/issues/4
?>
--EXPECTF--
