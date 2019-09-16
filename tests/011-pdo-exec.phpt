--TEST--
Calls to PDO::exec are logged
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
<?php if (!extension_loaded("PDO")) die("Skipped: PDO extension required."); ?>
<?php if (!extension_loaded("pdo_sqlite")) die("Skipped: pdo_sqlite extension required."); ?>
--FILE--
<?php
$dbh = new PDO('sqlite::memory:');
echo $dbh->exec("CREATE TABLE foo (col INT PRIMARY KEY)");
echo $dbh->exec("INSERT INTO foo (col) VALUES (1), (2) ");
echo "\n";

$calls = scoutapm_get_calls();
var_dump($calls[0]['function']);
var_dump($calls[0]['argv'][0]);
var_dump($calls[1]['function']);
var_dump($calls[1]['argv'][0]);
?>
--EXPECTF--
02
string(9) "PDO->exec"
string(38) "CREATE TABLE foo (col INT PRIMARY KEY)"
string(9) "PDO->exec"
string(38) "INSERT INTO foo (col) VALUES (1), (2) "
