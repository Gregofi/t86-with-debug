#!/bin/bash

failed=0
for f in `find t86 common debugger t86-cli t86-parser tests -name "*.cpp" -o -name "*.h"`; do
    clang-format --dry-run -Werror $f
    if [[ $? -ne 0 ]]; then
        failed=1
    fi
done

exit ${failed}
