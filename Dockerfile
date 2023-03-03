FROM ubuntu:22.04

RUN apt-get update -y && \
    apt-get install -y \
        gcc \
        g++ \
        cmake \
        make \
        clang-format \
        git

COPY t86 /main/t86
COPY tc /main/tc

