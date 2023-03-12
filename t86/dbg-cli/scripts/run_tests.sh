#!/bin/bash

if [[ "$#" -ne 1 ]]; then
    echo "Usage: run_tests.sh dbg-cli"
    exit 1
fi

for file in tests/*.in; do
    ref="${file%.in}.ref"
    conf="${file%.in}.conf"
    t86_file=tests/`cat ${conf}`
    ${1} ${t86_file} < ${file} | sed 's/\x1B\[[0-9;]\{1,\}[A-Za-z]//g' > "test_out.tmp"
    diff "test_out.tmp" ${ref} > "failed_diff.tmp"
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed"
        cat failed_diff.tmp
        exit 1
    fi
    rm failed_diff.tmp test_out.tmp
done

echo "All tests passed :-)"
