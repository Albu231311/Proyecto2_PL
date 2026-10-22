# Proyecto #2 Parte 2 - HPC con OpenMPI colectivo

**Equipo 1:** _Camila Richter 23183, Marinés García 23391 y Carlos Alburez 231311_
**Fecha:** _(hoy)_

## III. Fase de Validación: Descifrado Cruzado Local

| Equipo que eligieron | Texto | Palabra objetivo |
|---|---|---|
| Grupo 9 | `cifrado_grupo9.bin` (2,168 bytes cifrados en DES, publicado en Base64 en el foro) | computación paralela y distribuida |

| Log de tiempo y speedup |
|---|
| T_s (tiempo local, 1 máquina, N procesos) = **42.90 s** ($N=1$, 1 proceso en local) |
| Llave encontrada = **3141592** |
| ¿Texto plano se recuperó correctamente? **Sí** (texto de 352 palabras recuperado íntegramente) |

## IV. Virtualización del Entorno: Preparación con Docker

| Validar | Esperado | ¿Funcionó? | Qué se tuvo que modificar o hacer para lograrlo |
|---|---|---|---|
| `docker run --rm mpi-cluster-image mpicc --version` | gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 | Sí | Nada — salió `gcc (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0` (coincide, solo difiere el patch) |
| `docker run --rm mpi-cluster-image mpirun -np 2 echo "Proceso MPI ejecutado"` | imprime el mensaje 2 veces | Sí | Nada |
| `docker run -it --rm mpi-cluster-image /bin/bash` | entra a la terminal del contenedor | Sí | Nada |
| `service ssh start` | comienza un service ssh | Sí | Nada |
| `ssh-keygen -t rsa -N "" -f ~/.ssh/id_rsa` + `cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys` | genera una clave y se prueba localmente | Sí | Nada |
| `ssh localhost echo "Conexion SSH sin contrasena exitosa"` | imprime el mensaje | Sí | Nada |

_(Validado en macOS/Apple Silicon con Docker Desktop. El Dockerfile pasó las 6 pruebas a la primera, sin tener que modificar nada — PermitRootLogin, StrictHostKeyChecking y OMPI_ALLOW_RUN_AS_ROOT ya quedaron bien configurados desde el build.)_

## V. Despliegue de Red Física y Topología

**Nodo Maestro:** _(quién / IP)_
**Nodos Workers:** _(quiénes / IPs)_

### Paso A: Sincronización de llaves SSH inter-nodo

| Acción (desde el Nodo Maestro) | Resultado |
|---|---|
| Generar llave pública | |
| Copiarla al `authorized_keys` de cada Worker | |

### Paso B: Ejecución del Ataque Distribuido

Comando ejecutado (ajustar subred real):
```
mpirun --hostfile cluster_hosts --mca btl_tcp_if_include 192.168.1.0/24 \
  --mca oob_tcp_if_include 192.168.1.0/24 -np <TOTAL_SLOTS> \
  ./bruteforce <archivo_cifrado> "<palabra objetivo>"
```

- [ ] Captura de pantalla: comando ejecutado
- [ ] Captura de pantalla: salida mostrando distintos nodos (IPs) participando
- [ ] Captura de pantalla: mensaje final con la llave encontrada

## VI. Discusión de Resultados y Conclusiones

**1. ¿Por qué OpenMPI se congela indefinidamente sin `--mca btl_tcp_if_include`? Expliquen mecánicamente qué ocurre con docker0 en múltiples máquinas.**

> _(respuesta)_

**2. Problemas de red que enfrentaron:**

> _(respuesta)_

**3. Tiempo local (T_s) vs tiempo en el clúster físico:**

> _(respuesta — tabla o prosa)_

**4. ¿Qué pasa si configuran el hostfile para 32 procesos MPI en una máquina con solo 4 núcleos físicos?**

> _(respuesta)_

**5. Vulnerabilidades y retos logísticos de administrar HPC con hardware heterogéneo interconectado:**

> _(respuesta)_

**6. ¿Lograron el objetivo? ¿Qué parte les gustó más / menos? ¿Qué habrían hecho mejor?**

> _(respuesta)_
