--TEST--
Predis userland functions are supported
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php

// Mimicks the implementation of Predis\Client (uses magic methods for set/get)
namespace Predis {
  class Client {
    private $fh;
    public function __construct() {
      $this->fh = fopen("php://memory", "w+");
    }
    public function __call($method, $argv) {
      if ($method === 'get') {
        rewind($this->fh);
        return fread($this->fh, 3);
      }
      if ($method === 'del') {
        ftruncate($this->fh, 0);
        return;
      }
      fwrite($this->fh, $argv[1]);
    }
  }
}

namespace {
  $client = new \Predis\Client();
  $client->set('foo', 'bar');
  var_dump($client->get('foo'));
  $client->del('foo');
  var_dump($client->get('foo'));

  $calls = scoutapm_get_calls();

  var_dump(array_column($calls, 'function'));
  var_dump(array_column(array_column($calls, 'argv'), 0));
}

?>
--EXPECTF--
string(%s) "bar"
string(0) ""
array(4) {
  [0]=>
  string(18) "Predis\Client->set"
  [1]=>
  string(18) "Predis\Client->get"
  [2]=>
  string(18) "Predis\Client->del"
  [3]=>
  string(18) "Predis\Client->get"
}
array(4) {
  [0]=>
  string(3) "set"
  [1]=>
  string(3) "get"
  [2]=>
  string(3) "del"
  [3]=>
  string(3) "get"
}
