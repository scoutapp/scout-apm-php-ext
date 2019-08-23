--TEST--
Class without a defined constructor does not crash
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
--FILE--
<?php
class Foo {}
$x = new Foo();
echo "End.\n";
?>
--EXPECTF--
End.
