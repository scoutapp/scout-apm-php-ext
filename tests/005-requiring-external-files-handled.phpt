--TEST--
Requires to external files do not crash
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
--FILE--
<?php
require 'external.php';
echo "End.\n";
?>
--EXPECTF--
External file has been included.
End.
