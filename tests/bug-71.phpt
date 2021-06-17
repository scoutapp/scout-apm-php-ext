--TEST--
Bug https://github.com/scoutapp/scout-apm-php-ext/issues/71 - only record arguments for prepare if it returns an object
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php if (!extension_loaded("PDO")) die("skip PDO extension required."); ?>
<?php if (!extension_loaded("pdo_sqlite")) die("skip pdo_sqlite extension required."); ?>
--FILE--
<?php
scoutapm_enable_instrumentation(true);

$dbh = new PDO('sqlite::memory:');
$dbh->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
$stmt = $dbh->prepare("SELECT nonexist FROM nonexist");

?>
--EXPECTF--
Warning: PDO::prepare(): SQLSTATE[HY000]: General error: 1 no such table: nonexist in %s
