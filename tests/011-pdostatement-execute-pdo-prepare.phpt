--TEST--
Calls to PDOStatement::execute are logged when created from PDO->prepare()
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php if (!extension_loaded("PDO")) die("skip PDO extension required."); ?>
<?php if (!extension_loaded("pdo_sqlite")) die("skip pdo_sqlite extension required."); ?>
--FILE--
<?php
$dbh = new PDO('sqlite::memory:');
$stmt1 = $dbh->prepare("SELECT 1 + 2");
$stmt2 = $dbh->prepare("SELECT 3 + 4");
$stmt1->execute();
$stmt2->execute();

$calls = scoutapm_get_calls();
var_dump($calls[0]['function']);
var_dump($calls[0]['argv']);
var_dump($calls[1]['function']);
var_dump($calls[1]['argv']);
?>
--EXPECTF--
string(21) "PDOStatement->execute"
array(1) {
  [0]=>
  string(%d) "SELECT 1 + 2"
}
string(21) "PDOStatement->execute"
array(1) {
  [0]=>
  string(%d) "SELECT 3 + 4"
}
