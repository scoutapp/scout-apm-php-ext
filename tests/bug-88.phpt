--TEST--
Bug https://github.com/scoutapp/scout-apm-php-ext/issues/88 - memory usage should not increase when no instrumentation happens
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
<?php /* PHP_OS_FAMILY === "Windows" - needs PHP 7.2+ */ if (stripos(PHP_OS, 'Win') === 0) die("skip not for Windows."); ?>
--FILE--
<?php

class A {
  function b() {}
}

$a = new A();

scoutapm_enable_instrumentation(true);

$before = (int) (exec("ps --pid " . getmypid() . " --no-headers -o rss"));
for ($i = 0; $i <= 10000000; $i++) {
    $a->b();
}
$after = (int) (exec("ps --pid " . getmypid() . " --no-headers -o rss"));

echo "Before & after:\n";
var_dump($before, $after);

echo "Difference:\n";
var_dump($after - $before);

$threshold = 500; // bytes
echo "Within $threshold bytes limit:\n";
var_dump(($after - $before) < $threshold);

?>
--EXPECTF--
Before & after:
int(%d)
int(%d)
Difference:
int(%d)
Within %d bytes limit:
bool(true)
