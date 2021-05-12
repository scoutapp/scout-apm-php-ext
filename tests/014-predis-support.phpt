--TEST--
Predis userland functions are supported
--SKIPIF--
<?php
if (!extension_loaded("scoutapm")) die("skip scoutapm extension required.");
if (shell_exec("which composer") === null) die("skip composer not found in path.");

$out = null;
$result = null;
exec("mkdir -p /tmp/scout_predis_test && cd /tmp/scout_predis_test && composer require -n predis/predis", $out, $result);

if ($result !== 0) {
  die("skip composer failed: " . implode(", ", $out));
}

require "/tmp/scout_predis_test/vendor/autoload.php";

// Check Redis is running & can connect to it
// Run with: docker run --rm --name redis -p 6379:6379 -d redis
$client = new \Predis\Client();
try {
  $client->connect();
} catch (\Predis\Connection\ConnectionException $e) {
  die("skip " . $e->getMessage());
}
?>
--FILE--
<?php

require "/tmp/scout_predis_test/vendor/autoload.php";

$client = new \Predis\Client();

$client->set('foo', 'bar');
var_dump($client->get('foo'));
$client->append('foo', 'baz');
$client->del('foo');

$client->set('count', 0);
$client->incr('count');
$client->decr('count');
$client->incrBy('count', 2);
$client->decrBy('count', 2);

$calls = scoutapm_get_calls();

var_dump(array_column($calls, 'function'));

?>
--CLEAN--
<?php
shell_exec("rm -Rf /tmp/scout_predis_test");
?>
--EXPECTF--
string(%s) "bar"
array(%d) {
  [%d]=>
  string(%d) "Predis\Client->set"
  [%d]=>
  string(%d) "Predis\Client->get"
  [%d]=>
  string(%d) "Predis\Client->append"
  [%d]=>
  string(%d) "Predis\Client->del"
  [%d]=>
  string(%d) "Predis\Client->set"
  [%d]=>
  string(%d) "Predis\Client->incr"
  [%d]=>
  string(%d) "Predis\Client->decr"
  [%d]=>
  string(%d) "Predis\Client->incrBy"
  [%d]=>
  string(%d) "Predis\Client->decrBy"
}
