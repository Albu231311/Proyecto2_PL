# Proyecto #2, Fase 2 (CC3069) - imagen para el cluster MPI vía Docker
#
# bruteforce.c usa <openssl/des.h> directo (sin libreria rpc/des_crypt.h),
# por eso solo necesitamos libssl-dev + enlazar con -lcrypto.

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

# 1. Dependencias base (build-essential, openmpi-bin, libopenmpi-dev,
#    openssh-server son las obligatorias del enunciado; libssl-dev es
#    la que agregamos para compilar el DES de bruteforce.c via OpenSSL;
#    openssh-client + iproute2/iputils-ping/net-tools ayudan al
#    troubleshooting de red del punto V).
RUN apt-get update && apt-get install -y \
    build-essential \
    openmpi-bin \
    libopenmpi-dev \
    openssh-server \
    openssh-client \
    libssl-dev \
    iproute2 \
    iputils-ping \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# 2. SSH sin intervencion manual
RUN mkdir -p /var/run/sshd && \
    ssh-keygen -A && \
    sed -i '/^#\?PermitRootLogin/c\PermitRootLogin yes' /etc/ssh/sshd_config && \
    echo "StrictHostKeyChecking no" >> /etc/ssh/ssh_config

# 3. Codigo fuente + compilacion
WORKDIR /mpi
COPY . /mpi

RUN mpicc -O3 -Wno-deprecated-declarations bruteforce.c -o bruteforce -lcrypto

EXPOSE 22
CMD ["/bin/bash"]
