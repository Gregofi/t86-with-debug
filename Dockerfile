FROM debian

RUN apt-get update -y && \
    apt-get install -y gcc g++ make cmake git

COPY t86 t86
