--TEST--
Both URL and Method can be captured using file_get_contents
--SKIPIF--
<?php if (!extension_loaded("scoutapm")) die("skip scoutapm extension required."); ?>
--FILE--
<?php

var_dump(in_array('file_get_contents', scoutapm_list_instrumented_functions()));
scoutapm_enable_instrumentation(true);

@file_get_contents(
    'https://scoutapm.com/robots.txt',
    false,
    stream_context_create([
        'http' => [
            'method' => 'POST',
            'header' => [
                'User-Agent: scoutapp/scout-apm-php-ext Test Suite FGC',
            ],
        ],
    ])
);
$call = scoutapm_get_calls()[0];

var_dump($call['function']);
var_dump($call['argv'][0]);
var_dump(json_decode($call['argv'][2], true)['http']['method']);
?>
--EXPECTF--
bool(true)
string(%d) "file_get_contents"
string(%d) "https://scoutapm.com/robots.txt"
string(%d) "POST"
