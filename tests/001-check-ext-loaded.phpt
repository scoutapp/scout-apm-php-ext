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

var_dump(extension_loaded('scoutapm'));
?>
--EXPECTF--
string(%d) "    with scoutapm v%s, Copyright %d, by Scout APM"
bool(true)
