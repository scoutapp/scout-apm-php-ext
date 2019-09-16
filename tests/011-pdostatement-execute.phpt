--TEST--
Calls to PDOStatement::execute are logged
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
<?php if (!extension_loaded("PDO")) die("Skipped: PDO extension required."); ?>
<?php if (!extension_loaded("pdo_sqlite")) die("Skipped: pdo_sqlite extension required."); ?>
--FILE--
<?php
$dbh = new PDO('sqlite::memory:');
$stmt = $dbh->prepare("SELECT 1 + 2");
$stmt->execute();
var_dump($stmt->fetch(PDO::FETCH_ASSOC));

$calls = scoutapm_get_calls();
var_dump($calls[0]['function']);
?>
--EXPECTF--
array(1) {
  ["1 + 2"]=>
  string(1) "3"
}
string(21) "PDOStatement->execute"
