--TEST--
Elasticsearch userland functions are supported
--SKIPIF--
<?php
if (!extension_loaded("scoutapm")) die("skip scoutapm extension required.");
if (!extension_loaded("curl")) die("skip Elasticsearch needs the curl extension.");
if (shell_exec("which composer") === null) die("skip composer not found in path.");

$out = null;
$result = null;
exec("mkdir -p /tmp/scout_elastic_test && cd /tmp/scout_elastic_test && composer require -n elasticsearch/elasticsearch:^8.0", $out, $result);

if ($result !== 0) {
  die("skip composer failed: " . implode(", ", $out));
}

if (!getenv('CI')) {
    require "/tmp/scout_elastic_test/vendor/autoload.php";

    // Check Elasticsearch is running & can connect to it
    /* Run with:
docker run --rm --name elasticsearch \
  -p 9200:9200 \
  -e discovery.type=single-node \
  -e xpack.security.enabled=false \
  -e xpack.security.enrollment.enabled=false \
  -e xpack.security.http.ssl.enabled=false \
  -e xpack.security.transport.ssl.enabled=false \
  elasticsearch:8.1.2
    */
    $client = \Elastic\Elasticsearch\ClientBuilder::create()
        ->setHosts(['localhost:9200'])
        ->build();
    try {
        $client->search([]);
    } catch (\Elastic\Elasticsearch\Common\Exceptions\NoNodesAvailableException $e) {
      die("skip " . $e->getMessage());
    }
}
?>
--FILE--
<?php

echo implode("\n", array_intersect(
    [
        'Elastic\Elasticsearch\Client->index',
        'Elastic\Elasticsearch\Client->get',
        'Elastic\Elasticsearch\Client->search',
        'Elastic\Elasticsearch\Client->delete',
    ],
    scoutapm_list_instrumented_functions()
)) . "\n";
scoutapm_enable_instrumentation(true);

require "/tmp/scout_elastic_test/vendor/autoload.php";

$client = \Elastic\Elasticsearch\ClientBuilder::create()
    ->setHosts(['localhost:9200'])
    ->build();

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
Elastic\Elasticsearch\Client->index
Elastic\Elasticsearch\Client->get
Elastic\Elasticsearch\Client->search
Elastic\Elasticsearch\Client->delete
array(%d) {
  [%d]=>
  string(%d) "Elastic\Elasticsearch\Client->index"
  [%d]=>
  string(%d) "Elastic\Elasticsearch\Client->get"
  [%d]=>
  string(%d) "Elastic\Elasticsearch\Client->search"
  [%d]=>
  string(%d) "Elastic\Elasticsearch\Client->delete"
}
