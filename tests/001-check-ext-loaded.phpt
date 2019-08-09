--TEST--
Check Scout APM extension is loaded
--FILE--
<?php
ob_start();
phpinfo(INFO_GENERAL);
$phpinfo = ob_get_clean();

foreach (explode("\n", $phpinfo) as $line) {
  if (stripos($line, '    with scoutapm') === 0) {
    var_dump($line);
  }
}

// @todo check why isn't this bool(true), maybe only works for modules, not Zend extensions?
//var_dump(extension_loaded('scoutapm'));
?>
--EXPECT--
string(57) "    with scoutapm v0.0, Copyright Scout APM, by Scout APM"
