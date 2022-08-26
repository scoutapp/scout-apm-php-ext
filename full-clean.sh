#!/usr/bin/env bash

set -euxo pipefail

make clean
phpize --clean
git clean -xdn -e .idea
