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
        'memcached->get',
        'memcached->set',
    ],
    scoutapm_list_instrumented_functions()
)) . "\n";

$m = new Memcached();
$m->addServer('localhost', 11211);
$m->set('foo', 'bar');
var_dump($m->get('foo'));

$calls = scoutapm_get_calls();

var_dump(array_column($calls, 'function'));

?>
--EXPECTF--
memcached->get
memcached->set
string(%s) "bar"
array(%d) {
  [%d]=>
  string(%d) "Memcached->set"
  [%d]=>
  string(%d) "Memcached->get"
}
