--TEST--
Executing a closure/anonymous function does not crash
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
--FILE--
<?php
eval('echo "Evaled code called.\n";');
echo "End.\n";
?>
--EXPECTF--
Evaled code called.
End.
