--TEST--
Executing a closure/anonymous function does not crash
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
--FILE--
<?php
$func = function () {
  echo "Anonymous function called.\n";
};
$func();
echo "End.\n";
?>
--EXPECTF--
Anonymous function called.
End.
