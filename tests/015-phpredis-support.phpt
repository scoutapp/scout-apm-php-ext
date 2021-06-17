--TEST--
PHP Redis C extension functions are instrumented
--SKIPIF--
<?php
if (!extension_loaded("scoutapm")) die("skip scoutapm extension required.");
if (!extension_loaded("redis")) die("skip redis extension required.");

if (!getenv('CI')) {
    // Check Redis is running & can connect to it
    // Run with: docker run --rm --name redis -p 6379:6379 -d redis
    $client = new Redis();
    try {
      $client->connect('127.0.0.1', 6379);
    } catch (\RedisException $e) {
      die("skip " . $e->getMessage());
    }
}
?>
--FILE--
<?php

echo implode("\n", array_intersect(
    [
        'redis->append',
        'redis->decr',
        'redis->decrby',
        'redis->get',
        'redis->getbit',
        'redis->getrange',
        'redis->getset',
        'redis->incr',
        'redis->incrby',
        'redis->mget',
        'redis->mset',
        'redis->msetnx',
        'redis->set',
        'redis->setbit',
        'redis->setex',
        'redis->psetex',
        'redis->setnx',
        'redis->setrange',
        'redis->strlen',
        'redis->del',
    ],
    scoutapm_list_instrumented_functions()
)) . "\n";
scoutapm_enable_instrumentation(true);

$client = new Redis();
$client->connect('127.0.0.1', 6379);

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
--EXPECTF--
redis->append
redis->decr
redis->decrby
redis->get
redis->getbit
redis->getrange
redis->getset
redis->incr
redis->incrby
redis->mget
redis->mset
redis->msetnx
redis->set
redis->setbit
redis->setex
redis->psetex
redis->setnx
redis->setrange
redis->strlen
redis->del
string(%s) "bar"
array(%d) {
  [%d]=>
  string(%d) "Redis->set"
  [%d]=>
  string(%d) "Redis->get"
  [%d]=>
  string(%d) "Redis->append"
  [%d]=>
  string(%d) "Redis->del"
  [%d]=>
  string(%d) "Redis->getSet"
  [%d]=>
  string(%d) "Redis->getRange"
  [%d]=>
  string(%d) "Redis->setRange"
  [%d]=>
  string(%d) "Redis->setex"
  [%d]=>
  string(%d) "Redis->psetex"
  [%d]=>
  string(%d) "Redis->setnx"
  [%d]=>
  string(%d) "Redis->strlen"
  [%d]=>
  string(%d) "Redis->set"
  [%d]=>
  string(%d) "Redis->incr"
  [%d]=>
  string(%d) "Redis->decr"
  [%d]=>
  string(%d) "Redis->incrBy"
  [%d]=>
  string(%d) "Redis->decrBy"
  [%d]=>
  string(%d) "Redis->mset"
  [%d]=>
  string(%d) "Redis->msetnx"
  [%d]=>
  string(%d) "Redis->mget"
  [%d]=>
  string(%d) "Redis->set"
  [%d]=>
  string(%d) "Redis->setBit"
  [%d]=>
  string(%d) "Redis->getBit"
}
