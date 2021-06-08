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

if (!getenv('CI')) {
    require "/tmp/scout_predis_test/vendor/autoload.php";

    // Check Redis is running & can connect to it
    // Run with: docker run --rm --name redis -p 6379:6379 -d redis
    $client = new \Predis\Client();
    try {
      $client->connect();
    } catch (\Predis\Connection\ConnectionException $e) {
      die("skip " . $e->getMessage());
    }
}
?>
--FILE--
<?php

echo implode("\n", array_intersect(
    [
        'Predis\Client->append',
        'Predis\Client->decr',
        'Predis\Client->decrBy',
        'Predis\Client->get',
        'Predis\Client->getBit',
        'Predis\Client->getRange',
        'Predis\Client->getSet',
        'Predis\Client->incr',
        'Predis\Client->incrBy',
        'Predis\Client->mGet',
        'Predis\Client->mSet',
        'Predis\Client->mSetNx',
        'Predis\Client->set',
        'Predis\Client->setBit',
        'Predis\Client->setEx',
        'Predis\Client->pSetEx',
        'Predis\Client->setNx',
        'Predis\Client->setRange',
        'Predis\Client->strlen',
        'Predis\Client->del',
    ],
    scoutapm_list_instrumented_functions()
)) . "\n";

require "/tmp/scout_predis_test/vendor/autoload.php";

$client = new \Predis\Client();

// Simple operations
$client->set('foo', 'bar');
var_dump($client->get('foo'));
$client->append('foo', 'baz');
$client->del('foo');
$client->getSet('foo', 'bat');
$client->getRange('foo', 0, 2);
$client->setRange('foo', 0, 'qux');
$client->setEx('expire1', 1, 'value1');
$client->pSetEx('expire2', 1, 'value2');
$client->setNx('fuu', 'new');
$client->strlen('fuu');

// Increment/Decrement
$client->set('count', 0);
$client->incr('count');
$client->decr('count');
$client->incrBy('count', 2);
$client->decrBy('count', 2);

// Multi-operations
$client->mSet(['a' => 'a', 'b' => 'b']);
$client->mSetNx(['c' => 'c', 'd' => 'd']);
$client->mGet(['a', 'b', 'c', 'd']);

// Bit operations
$client->set('bit', 0);
$client->setBit('bit', 8, 1);
$client->getBit('bit', 8);

$calls = scoutapm_get_calls();

var_dump(array_column($calls, 'function'));

?>
--CLEAN--
<?php
shell_exec("rm -Rf /tmp/scout_predis_test");
?>
--EXPECTF--
Predis\Client->append
Predis\Client->decr
Predis\Client->decrBy
Predis\Client->get
Predis\Client->getBit
Predis\Client->getRange
Predis\Client->getSet
Predis\Client->incr
Predis\Client->incrBy
Predis\Client->mGet
Predis\Client->mSet
Predis\Client->mSetNx
Predis\Client->set
Predis\Client->setBit
Predis\Client->setEx
Predis\Client->pSetEx
Predis\Client->setNx
Predis\Client->setRange
Predis\Client->strlen
Predis\Client->del
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
  string(%d) "Predis\Client->getSet"
  [%d]=>
  string(%d) "Predis\Client->getRange"
  [%d]=>
  string(%d) "Predis\Client->setRange"
  [%d]=>
  string(%d) "Predis\Client->setEx"
  [%d]=>
  string(%d) "Predis\Client->pSetEx"
  [%d]=>
  string(%d) "Predis\Client->setNx"
  [%d]=>
  string(%d) "Predis\Client->strlen"
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
  [%d]=>
  string(%d) "Predis\Client->mSet"
  [%d]=>
  string(%d) "Predis\Client->mSetNx"
  [%d]=>
  string(%d) "Predis\Client->mGet"
  [%d]=>
  string(%d) "Predis\Client->set"
  [%d]=>
  string(%d) "Predis\Client->setBit"
  [%d]=>
  string(%d) "Predis\Client->getBit"
}
