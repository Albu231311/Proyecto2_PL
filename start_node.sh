#!/bin/bash
# =============================================================================
# start_node.sh — Script de arranque para cada nodo del súper-clúster
# Proyecto #2, Fase 3 (CC3069) — Computación Paralela y Distribuida
#
# Uso DENTRO del contenedor Docker:
#   chmod +x /mpi/start_node.sh
#   /mpi/start_node.sh <MI_IP>
#
# Ejemplos (IPs Tailscale del Súper-Clúster):
#   /mpi/start_node.sh 100.115.39.25   # Carlos (Maestro)
#   /mpi/start_node.sh 100.106.125.75  # Cami
#   /mpi/start_node.sh 100.82.62.97    # Marines
#   /mpi/start_node.sh 100.83.204.40   # Bran
#   /mpi/start_node.sh 100.80.207.53   # Anggie
#   /mpi/start_node.sh 100.73.26.112   # Iris
# =============================================================================

set -e

MY_IP="$1"
if [ -z "$MY_IP" ]; then
  echo "=========================================="
  echo " ERROR: Debes pasar tu IP como argumento"
  echo " Uso: ./start_node.sh <TU_IP>"
  echo ""
  echo " Ejemplos:"
  echo "   ./start_node.sh 100.115.39.25   (Carlos)"
  echo "   ./start_node.sh 100.106.125.75  (Cami)"
  echo "   ./start_node.sh 100.82.62.97    (Marines)"
  echo "   ./start_node.sh 100.83.204.40   (Bran)"
  echo "   ./start_node.sh 100.80.207.53   (Anggie)"
  echo "   ./start_node.sh 100.73.26.112   (Iris)"
  echo "=========================================="
  exit 1
fi

echo ""
echo ">>> [1/3] Iniciando servicio SSH..."
service ssh start

echo ""
echo ">>> [2/3] Agregando llave publica del Maestro (Carlos)..."
MASTER_KEY="ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQDVHPdjZ8GvzSwcRjrdRvPgoj4nciSIVkTe3ohhHQ87tMiEqZ0tMKPRTftnnNgkHoUKnRKfWlHAyGIYOECkE3HvNIsZdFJg2bJ0I6wuy8E4DMN2UNg+//rFfl+RlVlZQGyq5Yr1J/9+Ku9iNHSeuPJq9M4iwG3Fp/0Z2jfIe0v9hbUCJnOUU5sqQ9hqYpRLl15DvtogbzPy4PWCnsWQFOfjXI7Acs3skJJg30liPnmDiFQPS0Q8i0Uk0sfUnwz0T/6Vb8y+jQairnwuYs8Nzzt3md7BIeaXulEGIuBBpDMzP5wmhOFj5/d0D6aorCEY7yknbhrOFUOYIkh8vcLyf1de84t82CqFgUQgP88JMHkKic7gv4KfrbpQjx0YOE0kCUUP8SLbX0c5lmuZQ/Vz576v+2tMMuL3vwdgPmC02zMnjbW5GHOCfRlKVIVHbFeeG8TZ5gWWoEzaUr7KsyW9WZywf61n1o4N2hHM+V5YQ/Ct1l+47G7ODCplc3Uc8zcHGLc= root@buildkitsandbox"

if grep -q "buildkitsandbox" /root/.ssh/authorized_keys 2>/dev/null; then
  echo "    (Llave ya existe, no se duplica)"
else
  echo "$MASTER_KEY" >> /root/.ssh/authorized_keys
  echo "    Llave agregada exitosamente."
fi

echo ""
echo ">>> [3/3] Creando interfaz virtual cluster0 con IP ${MY_IP}/32..."
ip link del cluster0 2>/dev/null || true
ip link add cluster0 type dummy
ip addr add ${MY_IP}/32 dev cluster0
ip link set cluster0 up

echo ""
echo "============================================="
echo "  NODO LISTO PARA EL CLUSTER"
echo "============================================="
echo "  IP asignada:  $MY_IP"
echo "  SSH:          $(service ssh status 2>&1 | head -1)"
echo "  cluster0:     $(ip -4 addr show cluster0 | grep inet | awk '{print $2}')"
echo "============================================="
echo ""
echo "  NO CIERRES ESTA TERMINAL."
echo "  Espera a que Carlos ejecute mpirun."
echo ""

