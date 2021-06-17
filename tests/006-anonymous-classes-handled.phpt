--TEST--
Executing a closure/anonymous function does not crash
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php
scoutapm_enable_instrumentation(true);
$func = function () {
  echo "Anonymous function called.\n";
};
$func();
echo "End.\n";
?>
--EXPECTF--
Anonymous function called.
End.
