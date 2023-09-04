#!/bin/bash

function assert() {
    expected="$1"
    input="$2"

    ./main "$input" > tmp.s || exit 1
    gcc -o tmp tmp.s || exit 1
    ./tmp; actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "$input => $actual, expected $expected" 1>&2
        echo 'Failed!'
        exit 1
    fi

    echo "$input => $actual"
}

assert 0  '0'
assert 42 '42'
assert 21 '5+20-4'
assert 41 '12 + 34 - 5'
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4  '(3+5)/2'
echo 'Success!'
