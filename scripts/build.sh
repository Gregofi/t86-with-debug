#!/bin/bash

function build {
    cd ${1}
    ./scripts/build.sh
    cd ..
}

build "t86"
build "tc"
