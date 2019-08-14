#!/usr/bin/env bash

set -euo pipefail

PHP=`which php`
PHP_DESCRIPTION="without scoutapm extension"
PASSES=20
ITERATIONS_PER_PASS=100000

function usage {
  echo "$0 [-w] [-p N] [-i N]"
  echo ""
  echo "  -w, --with-scoutapm                        enables ScoutAPM extension for benchmark"
  echo "  -p 10, --passes 10                         run 10 passes for average (default=$PASSES)"
  echo "  -i 100000, --iterations-per-pass 100000    run 100000 iterations per pass (default=$ITERATIONS_PER_PASS)"
}

while [[ $# -gt 0 ]]
do
  key="$1"
  case $key in
      -w|--with-scoutapm)
      PHP="`which php` -d zend_extension=`pwd`/modules/scoutapm.so"
      PHP_DESCRIPTION="with scoutapm extension ENABLED"
      shift
      ;;
      -p|--passes)
      PASSES="$2"
      shift # past argument
      shift # past value
      ;;
      -i|--iterations-per-pass)
      ITERATIONS_PER_PASS="$2"
      shift # past argument
      shift # past value
      ;;
      *)    # unknown option
      echo "Unknown option $key."
      usage
      exit
      ;;
  esac
done

echo "Running $PASSES containing $ITERATIONS_PER_PASS iterations each, $PHP_DESCRIPTION..."

PHP_SCRIPT=`mktemp`
echo "<?php for(\$i =0; \$i < $ITERATIONS_PER_PASS; \$i++) { file_get_contents('zend_scoutapm.c'); }" > $PHP_SCRIPT

EXECUTION_TIMES_FILE=`mktemp`
for (( i=0; i <= $PASSES; i++)) do
  printf "."
  command time --format="%e" -ao $EXECUTION_TIMES_FILE $PHP $PHP_SCRIPT
done

printf "\n\n"

awk '{s+=$1}END{print "Average execution time:",s/NR}' RS=" " $EXECUTION_TIMES_FILE
unlink $EXECUTION_TIMES_FILE
unlink $PHP_SCRIPT
