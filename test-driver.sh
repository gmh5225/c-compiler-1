#!/usr/bin/env bash

tmp=`mktemp -d /tmp/XXXXXX`
trap 'rm -rf $tmp' EXIT
echo > $tmp/empty.c

check() {
    if [ $? -ne 0 ]; then
        echo "testing $1 ... failed"
        exit 1
    fi

    echo "testing $1 ... passed"
}

# `-o` option
rm -f $tmp/out
./main -o $tmp/out $tmp/empty.c
test -f $tmp/out
check '-o'

# `--help` option

./main --help 2>&1 | grep -q 'Usage:'
check '--help'

echo 'Success!'
