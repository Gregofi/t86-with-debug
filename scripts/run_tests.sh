#!/bin/bash

if [[ "$#" -ne 2 ]]; then
    echo "usage: run_tests.sh tinyc-compiler t86-cli"
    exit 1
fi

set -e
set -o xtrace

mkdir -p tests/out
mkdir -p tests/diff

for file in tests/*.tc; do
    BS=`basename $file`
    ref="tests/expected/${BS%.tc}.ref"
    out="tests/out/${BS%.tc}"
    diff="tests/diff/${BS%.tc}.diff"

    ${1} ${file} -x -r > "${out}.t86"
    ${2} "${out}.t86" > "${out}.out"
    diff ${out}.out ${ref} > ${diff}
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed"
        cat diff
        exit 1
    fi
done

echo "All tests passed :-)"
