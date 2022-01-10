--TEST--
Calls to PDO::query are logged
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php if (!extension_loaded("PDO")) die("skip PDO extension required."); ?>
<?php if (!extension_loaded("pdo_sqlite")) die("skip pdo_sqlite extension required."); ?>
--FILE--
<?php

var_dump(in_array('pdo->query', scoutapm_list_instrumented_functions()));
scoutapm_enable_instrumentation(true);

$dbh = new PDO('sqlite::memory:');
$stmt = $dbh->query("SELECT cast(1 + 2 AS text) AS result");
var_dump($stmt->fetch(PDO::FETCH_ASSOC));

$calls = scoutapm_get_calls();
var_dump($calls[0]['function']);
var_dump($calls[0]['argv'][0]);
?>
--EXPECTF--
bool(true)
array(%d) {
  ["result"]=>
  string(%d) "3"
}
string(%d) "PDO->query"
string(%d) "SELECT cast(1 + 2 AS text) AS result"
