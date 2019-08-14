--TEST--
Calls to file_get_contents are logged
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("Skipped: scoutapm extension required."); ?>
--FILE--
<?php
namespace SomeNamespace {
    function file_get_contents(string $_) {}

    echo "should not be logged: function defined in namespace, call implicitly resolves to namespaced function\n";
    file_get_contents(__FILE__);
    var_dump(scoutapm_get_calls());
}

namespace {
    echo "should not be logged - explicit, unimported call to another namespace\n";
    \SomeNamespace\file_get_contents(__FILE__);
    var_dump(scoutapm_get_calls());
}

namespace {
    use function SomeNamespace\file_get_contents;

    echo "should not be logged - imported call resolves to namespaced function\n";
    file_get_contents(__FILE__);
    var_dump(scoutapm_get_calls());
}

namespace {
    echo "SHOULD be logged\n";
    file_get_contents(__FILE__);
    var_dump(scoutapm_get_calls());
}
?>
--EXPECTF--
should not be logged: function defined in namespace, call implicitly resolves to namespaced function
array(0) {
}
should not be logged - explicit, unimported call to another namespace
array(0) {
}
should not be logged - imported call resolves to namespaced function
array(0) {
}
SHOULD be logged
array(1) {
  [0]=>
  array(4) {
    ["function"]=>
    string(17) "file_get_contents"
    ["entered"]=>
    float(%f)
    ["exited"]=>
    float(%f)
    ["time_taken"]=>
    float(%f)
  }
}
