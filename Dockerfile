# =============================================================================
# Proyecto #2, Fase 3 (CC3069) - Dockerfile Unificado para el Súper-Clúster
# Compatible con: macOS (Apple Silicon ARM64) y Windows (WSL2 / Docker Desktop x86_64)
# =============================================================================

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

# 1. Dependencias base obligatorias + herramientas de diagnóstico y normalización
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    openmpi-bin \
    libopenmpi-dev \
    openssh-server \
    openssh-client \
    libssl-dev \
    iproute2 \
    iputils-ping \
    net-tools \
    dos2unix \
    && rm -rf /var/lib/apt/lists/*

# 2. Configuración robusta de SSH para comunicación inter-nodo sin contraseña
RUN mkdir -p /var/run/sshd /root/.ssh && \
    ssh-keygen -A && \
    sed -i 's/#\?PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    printf '\nHost *\n    StrictHostKeyChecking no\n    UserKnownHostsFile /dev/null\n' >> /etc/ssh/ssh_config && \
    ssh-keygen -t rsa -N "" -f /root/.ssh/id_rsa && \
    cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys && \
    chmod 700 /root/.ssh && \
    chmod 600 /root/.ssh/authorized_keys

# 3. Estandarización de la ruta de trabajo en TODOS los nodos (Mac y Windows)
WORKDIR /mpi
COPY . /mpi

# 4. Normalizar saltos de línea (eliminar CRLF de Windows) y compilar binarios + helper LD_PRELOAD
RUN find /mpi -type f \( -name "*.c" -o -name "*.h" -o -name "*.sh" -o -name "cluster_hosts*" \) -exec dos2unix {} + 2>/dev/null || true && \
    mpicc -O3 -Wno-deprecated-declarations bruteforce.c -o bruteforce -lcrypto && \
    mpicc -O3 -Wno-deprecated-declarations bruteforce_masterworker.c -o bruteforce_mw -lcrypto && \
    gcc -shared -fPIC bind_any.c -o libbindany.so -ldl

EXPOSE 22
CMD ["/bin/bash"]
