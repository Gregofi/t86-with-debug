#!/bin/bash

if [[ "$#" -ne 1 ]]; then
    echo "Usage: run_tests.sh t86-cli"
    exit 1
fi

set -o xtrace

for file in t86-cli/tests/*.in; do
    ref="${file%.in}.ref"
    ${1} ${file} > "test_out.tmp"
    diff "test_out.tmp" "${file%.in}.ref" > "diff_out.tmp"
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed"
        cat diff.tmp
        exit 1
    fi
    rm diff_out.tmp test_out.tmp
done

echo "All tests passed :-)"
