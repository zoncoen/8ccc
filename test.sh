#!/bin/bash

function run {
    # int
    test 0 0
    test 42 42

    # boundary value
    test 2147483647 2147483647
    test -2147483648 -2147483648
}


function setup {
    make -s 8ccc
}

function teadown {
    rm -f tmp.out tmp.s
    echo "All tests passed"
}

function test {
    expected="$1"
    expr="$2"

    echo "$expr" | ./8ccc > tmp.s
    if [ ! $? ]; then
        echo "Failed to compile $expr"
        exit
    fi
    gcc -o tmp.out driver.c tmp.s || exit
    result="`./tmp.out`"
    if [ "$result" != "$expected" ]; then
        echo "Test failed: $expected expected but got $result"
        exit
    fi
}

setup

run

teadown

