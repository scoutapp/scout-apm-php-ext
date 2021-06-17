--TEST--
Elasticsearch userland functions are supported
--SKIPIF--
<?php
if (!extension_loaded("scoutapm")) die("skip scoutapm extension required.");
if (!extension_loaded("curl")) die("skip Elasticsearch needs the curl extension.");
if (shell_exec("which composer") === null) die("skip composer not found in path.");

$out = null;
$result = null;
exec("mkdir -p /tmp/scout_elastic_test && cd /tmp/scout_elastic_test && composer require -n elasticsearch/elasticsearch", $out, $result);

if ($result !== 0) {
  die("skip composer failed: " . implode(", ", $out));
}

if (!getenv('CI')) {
    require "/tmp/scout_elastic_test/vendor/autoload.php";

    // Check Elasticsearch is running & can connect to it
    // Run with: docker run --rm --name elasticsearch -p 9200:9200 -e discovery.type=single-node -d elasticsearch:7.13.1
    $client = \Elasticsearch\ClientBuilder::create()->build();
    try {
        $client->search([]);
    } catch (\Elasticsearch\Common\Exceptions\NoNodesAvailableException $e) {
      die("skip " . $e->getMessage());
    }
}
?>
--FILE--
<?php

echo implode("\n", array_intersect(
    [
        'Elasticsearch\Client->index',
        'Elasticsearch\Client->get',
        'Elasticsearch\Client->search',
        'Elasticsearch\Client->delete',
    ],
    scoutapm_list_instrumented_functions()
)) . "\n";
scoutapm_enable_instrumentation(true);

require "/tmp/scout_elastic_test/vendor/autoload.php";

$client = \Elasticsearch\ClientBuilder::create()->build();

$client->index(['index' => 'my_index', 'id' => 'my_id', 'body' => ['testField' => 'abc']]);
$client->get(['index' => 'my_index', 'id' => 'my_id']);
$client->search(['index' => 'my_index', 'body' => ['query' => ['match' => ['testField' => 'abc']]]]);
$client->delete(['index' => 'my_index', 'id' => 'my_id']);

$calls = scoutapm_get_calls();

var_dump(array_column($calls, 'function'));

?>
--CLEAN--
<?php
shell_exec("rm -Rf /tmp/scout_elastic_test");
?>
--EXPECTF--
Elasticsearch\Client->index
Elasticsearch\Client->get
Elasticsearch\Client->search
Elasticsearch\Client->delete
array(%d) {
  [%d]=>
  string(%d) "Elasticsearch\Client->index"
  [%d]=>
  string(%d) "Elasticsearch\Client->get"
  [%d]=>
  string(%d) "Elasticsearch\Client->search"
  [%d]=>
  string(%d) "Elasticsearch\Client->delete"
}
