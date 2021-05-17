--TEST--
Calls to PDO::exec are logged
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php if (!extension_loaded("PDO")) die("skip PDO extension required."); ?>
<?php if (!extension_loaded("pdo_sqlite")) die("skip pdo_sqlite extension required."); ?>
--FILE--
<?php

var_dump(in_array('pdo->exec', scoutapm_list_instrumented_functions()));

$dbh = new PDO('sqlite::memory:');
$dbh->exec("CREATE TABLE foo (col INT PRIMARY KEY)");
$dbh->exec("INSERT INTO foo (col) VALUES (1), (2) ");

$calls = scoutapm_get_calls();
var_dump($calls[0]['function']);
var_dump($calls[0]['argv'][0]);
var_dump($calls[1]['function']);
var_dump($calls[1]['argv'][0]);
?>
--EXPECTF--
bool(true)
string(9) "PDO->exec"
string(38) "CREATE TABLE foo (col INT PRIMARY KEY)"
string(9) "PDO->exec"
string(38) "INSERT INTO foo (col) VALUES (1), (2) "
