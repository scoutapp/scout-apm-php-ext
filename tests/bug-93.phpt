--TEST--
Bug https://github.com/scoutapp/scout-apm-php-ext/issues/93 - Should not segfault on static function usage
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php

scoutapm_enable_instrumentation(true);

(static function () {
    echo "Called 1.\n";
})();

$declared = static function () {
    echo "Called 2.\n";
};
$declared();

class A {
    static function thing() {
        echo "Called 3.\n";
    }
}
A::thing();

?>
--EXPECTF--
Called 1.
Called 2.
Called 3.
