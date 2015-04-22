#!/bin/bash

function run {
    # int
    test 0 0
    test 42 42

    # boundary value
    test 2147483647 2147483647
    test -2147483648 -2147483648

    # string
    test abc '"abc"'

    testfail '"abc'
    testfail '0abc'
}

function setup {
    make -s 8ccc
}

function teadown {
    echo "All tests passed"
}

function compile {
    echo "$1" | ./8ccc > tmp.s
    if [ $? -ne 0 ]; then
        echo "Faild to compile $1"
        exit
    fi
    gcc -o tmp.out driver.c tmp.s
    if [ $? -ne 0 ]; then
        echo "GCC failed"
        exit
    fi
}

function test {
    expected="$1"
    expr="$2"
    compile "$expr"
    result="`./tmp.out`"
    if [ "$result" != "$expected" ]; then
        echo "Test failed: $expected expected but got $result"
        exit
    fi
}

function testfail {
    expr="$1"
    echo "$expr" | ./8ccc > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Should fail to compile, but succeeded: $expr"
        exit
    fi
}

setup

run

teadown

