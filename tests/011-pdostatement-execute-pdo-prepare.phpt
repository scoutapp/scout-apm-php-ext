--TEST--
Calls to PDOStatement::execute are logged when created from PDO->prepare()
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php if (!extension_loaded("PDO")) die("skip PDO extension required."); ?>
<?php if (!extension_loaded("pdo_sqlite")) die("skip pdo_sqlite extension required."); ?>
--FILE--
<?php
$dbh = new PDO('sqlite::memory:');
$stmt = $dbh->prepare("SELECT 1 + 2");
$stmt->execute();
var_dump($stmt->fetch(PDO::FETCH_ASSOC));

$calls = scoutapm_get_calls();
var_dump($calls[0]['function']);
var_dump($calls[0]['argv']);
?>
--EXPECTF--
array(1) {
  ["1 + 2"]=>
  string(1) "3"
}
string(21) "PDOStatement->execute"
array(1) {
  [0]=>
  string(%d) "SELECT 1 + 2"
}
