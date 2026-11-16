# Build/run environment matching the paper's platform software stack
# (Ubuntu 20.04, gcc/g++ 9.4.0). See README.md, "Getting Started".
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential make cmake \
      libgoogle-perftools-dev google-perftools \
      libboost-dev libnuma-dev \
      unifdef cloc \
      python3 python3-matplotlib fonts-liberation \
      bc procps util-linux file gdb \
      git curl patch ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /pandora
