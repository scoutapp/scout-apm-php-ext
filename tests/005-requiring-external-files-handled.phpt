--TEST--
Requires to external files do not crash
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php
scoutapm_enable_instrumentation(true);
require 'external.inc';
echo "End.\n";
?>
--EXPECTF--
External file has been included.
End.
