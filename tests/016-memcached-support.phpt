--TEST--
Memcached C extension functions are instrumented
--SKIPIF--
<?php
if (!extension_loaded("scoutapm")) die("skip scoutapm extension required.");
if (!extension_loaded("memcached")) die("skip memcached extension required.");

if (!getenv('CI')) {
    // Check Memcached is running & can connect to it
    // Run with: docker run --rm --name memcached -p 11211:11211 -d memcached
    $m = new Memcached();
    $m->addServer('localhost', 11211);
    if (!$m->flush()) {
      die("skip Could not connect to Memcached - is it running?");
    }
}
?>
--FILE--
<?php

echo implode("\n", array_intersect(
    [
        'memcached->add',
        'memcached->addbykey',
        'memcached->append',
        'memcached->appendbykey',
        'memcached->cas',
        'memcached->decrement',
        'memcached->decrementbykey',
        'memcached->delete',
        'memcached->deletebykey',
        'memcached->deletemulti',
        'memcached->deletemultibykey',
        'memcached->flush',
        'memcached->get',
        'memcached->getallkeys',
        'memcached->getbykey',
        'memcached->getmulti',
        'memcached->getmultibykey',
        'memcached->increment',
        'memcached->incrementbykey',
        'memcached->prepend',
        'memcached->prependbykey',
        'memcached->replace',
        'memcached->replacebykey',
        'memcached->set',
        'memcached->setbykey',
        'memcached->setmulti',
        'memcached->setmultibykey',
    ],
    scoutapm_list_instrumented_functions()
)) . "\n";

$m = new Memcached();
$m->addServer('localhost', 11211);
$m->setOption(Memcached::OPT_COMPRESSION, false);

$m->set('foo', 'bar');
var_dump($m->get('foo'));
$m->append('foo', 'baz');
$m->prepend('foo', 'gaz');
$m->replace('foo', 'bar');
$m->cas(0, 'foo', 'bar');
$m->add('num', 1);
$m->decrement('num');
$m->increment('num');
$m->delete('num');
$m->setMulti(['a' => 'a', 'b' => 'b']);
$m->getMulti(['a', 'b']);
$m->deleteMulti(['a', 'b']);

$m->setByKey('key', 'foo', 'bar');
$m->getByKey('key', 'foo');
$m->appendByKey('key', 'foo', 'baz');
$m->prependByKey('key', 'foo', 'gaz');
$m->replaceByKey('key', 'foo', 'bar');
$m->casByKey(0, 'key', 'foo', 'bar');
$m->addByKey('key', 'num', 1);
$m->decrementByKey('key', 'num');
$m->incrementByKey('key', 'num');
$m->deleteByKey('key', 'num');
$m->setMultiByKey('key', ['a' => 'a', 'b' => 'b']);
$m->getMultiByKey('key', ['a', 'b']);
$m->deleteMultiByKey('key', ['a', 'b']);

$m->getAllKeys();
$m->flush();

$calls = scoutapm_get_calls();

var_dump(array_column($calls, 'function'));

?>
--EXPECTF--
memcached->add
memcached->addbykey
memcached->append
memcached->appendbykey
memcached->cas
memcached->decrement
memcached->decrementbykey
memcached->delete
memcached->deletebykey
memcached->deletemulti
memcached->deletemultibykey
memcached->flush
memcached->get
memcached->getallkeys
memcached->getbykey
memcached->getmulti
memcached->getmultibykey
memcached->increment
memcached->incrementbykey
memcached->prepend
memcached->prependbykey
memcached->replace
memcached->replacebykey
memcached->set
memcached->setbykey
memcached->setmulti
memcached->setmultibykey
string(%s) "bar"
array(%d) {
  [%d]=>
  string(%d) "Memcached->set"
  [%d]=>
  string(%d) "Memcached->get"
  [%d]=>
  string(%d) "Memcached->append"
  [%d]=>
  string(%d) "Memcached->prepend"
  [%d]=>
  string(%d) "Memcached->replace"
  [%d]=>
  string(%d) "Memcached->cas"
  [%d]=>
  string(%d) "Memcached->add"
  [%d]=>
  string(%d) "Memcached->decrement"
  [%d]=>
  string(%d) "Memcached->increment"
  [%d]=>
  string(%d) "Memcached->delete"
  [%d]=>
  string(%d) "Memcached->setMulti"
  [%d]=>
  string(%d) "Memcached->getMulti"
  [%d]=>
  string(%d) "Memcached->deleteMulti"
  [%d]=>
  string(%d) "Memcached->setByKey"
  [%d]=>
  string(%d) "Memcached->getByKey"
  [%d]=>
  string(%d) "Memcached->appendByKey"
  [%d]=>
  string(%d) "Memcached->prependByKey"
  [%d]=>
  string(%d) "Memcached->replaceByKey"
  [%d]=>
  string(%d) "Memcached->casByKey"
  [%d]=>
  string(%d) "Memcached->addByKey"
  [%d]=>
  string(%d) "Memcached->decrementByKey"
  [%d]=>
  string(%d) "Memcached->incrementByKey"
  [%d]=>
  string(%d) "Memcached->deleteByKey"
  [%d]=>
  string(%d) "Memcached->setMultiByKey"
  [%d]=>
  string(%d) "Memcached->getMultiByKey"
  [%d]=>
  string(%d) "Memcached->deleteMultiByKey"
  [%d]=>
  string(%d) "Memcached->getAllKeys"
  [%d]=>
  string(%d) "Memcached->flush"
}
