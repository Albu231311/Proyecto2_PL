# Proyecto #2 Parte 2 - HPC con OpenMPI colectivo

**Equipo 1:** _Camila Richter 23183, Marinés García 23391 y Carlos Alburez 231311_
**Fecha:** _26 de octubre de 2026_

## III. Fase de Validación: Descifrado Cruzado Local

| Equipo que eligieron | Texto | Palabra objetivo |
|---|---|---|
| Grupo 9 | `cifrado_grupo9.bin` (2,168 bytes cifrados en DES, publicado en Base64 en el foro) | computación paralela y distribuida |

| Log de tiempo y speedup |
|---|
| T_s (tiempo local, 1 máquina, N procesos) = **42.905149 s** ($N=1$, 1 proceso en local) |
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

**Red física:** Punto de acceso Wi-Fi dedicado (iPhone Hotspot) — Subred `172.20.10.0/28` (máscara `255.255.255.240`).

- **Nodo Maestro:** Carlos Alburez (231311) — IP: `172.20.10.2` (MacBook Apple Silicon, 4 slots)
- **Nodo Worker 1:** Camila Richter (23183) — IP: `172.20.10.5` (MacBook Apple Silicon, 4 slots)
- **Nodo Worker 2:** Marinés García (23391) — IP: `172.20.10.6` (MacBook Apple Silicon, 4 slots)
- **Capacidad total del clúster:** 12 núcleos físicos (`TOTAL_SLOTS = 12`).

### Paso A: Sincronización de llaves SSH inter-nodo

| Acción (desde el Nodo Maestro) | Resultado |
|---|---|
| Generar llave pública | Clave RSA de 3072 bits generada exitosamente en `~/.ssh/id_rsa.pub` en el contenedor maestro. |
| Copiarla al `authorized_keys` de cada Worker | Copiada y verificada sin contraseña. `ssh cami hostname` y `ssh marines hostname` respondieron ambas con `docker-desktop` de forma transparente. |

### Paso B: Ejecución del Ataque Distribuido

Comando ejecutado en el Nodo Maestro:
```bash
mpirun --hostfile cluster_hosts   --mca btl_tcp_if_include 172.20.10.0/28   --mca oob_tcp_if_include 172.20.10.0/28   -np 12 ./bruteforce cifrado_grupo9.bin "computación paralela y distribuida"
```

**Salida obtenida:**
- **Llave encontrada:** `3141592` (encontrada por el Proceso 0 en el nodo Maestro).
- **Tiempo de búsqueda en el clúster ($T_{\text{cluster}}$):** **`50.729352 segundos`**
- **Speedup del clúster ($S_{\text{cluster}}$):** `42.905149 s / 50.729352 s` = **`0.8458x`** (aproximadamente **`0.85x`**)
- **Eficiencia ($E$):** `(0.8458 / 12) * 100%` = **`7.05%`**
- **Texto descifrado:** Recuperado al 100% (352 palabras exactas del texto publicado por Grupo 9).

- [x] Captura de pantalla: comando ejecutado y 12 procesos inicializados
- [x] Captura de pantalla: salida mostrando los rangos de búsqueda asignados a cada nodo/proceso
- [x] Captura de pantalla: mensaje final con la llave encontrada (`3141592`), tiempo de búsqueda y texto descifrado

## VI. Discusión de Resultados y Conclusiones

**1. ¿Por qué OpenMPI se congela indefinidamente sin `--mca btl_tcp_if_include`? Expliquen mecánicamente qué ocurre con docker0 en múltiples máquinas.**

Mecánicamente, cuando OpenMPI inicializa su capa de transporte de datos BTL (*Byte Transfer Layer*) basada en TCP, consulta todas las interfaces de red reportadas por el sistema operativo mediante llamadas a `getifaddrs()`. En entornos donde se utiliza Docker, el motor crea interfaces de red tipo puente predeterminadas (`docker0` o `br-XXXX`) que utilizan rangos de subred idénticos (por ejemplo, `172.17.0.1/16` o `172.20.0.1/16`) en **todas** las máquinas de forma aislada.

Si no se restringe la subred explícitamente mediante `--mca btl_tcp_if_include`, el subsistema de direccionamiento de OpenMPI asume que la IP virtual de otra máquina es alcanzable a través de su propia interfaz virtual local. Al intentar enviar paquetes TCP a través de este puente inexistente entre equipos físicos, los paquetes entran en bucle local (*loopback*) o son descartados silenciosamente por el kernel. Como las primitivas colectivas de MPI (como `MPI_Barrier` y `MPI_Send`) dependen de sincronizaciones estrictas con confirmación de recepción (*handshake* TCP / ACK), los procesos del clúster quedan suspendidos indefinidamente en un bloqueo mutuo (*deadlock* de red o "MPI Hang") esperando una respuesta que la red física jamás transmitirá, sin reportar ningún error visible.

**2. Problemas de red que enfrentaron:**

1. **Aislamiento y NAT de la micro-VM de Docker Desktop en macOS:** A diferencia de Linux donde `--network host` enlaza directamente la tarjeta de red física, en macOS Docker Desktop corre sobre una máquina virtual interna LinuxKit. Esto provocó que la interfaz `eth0` de todas las computadoras del equipo tuviera exactamente la misma IP interna (`192.168.65.3`), originando colisiones de identidad (`received unexpected process identifier`) cuando los demonios intentaban comunicarse entre sí.
2. **Conflicto de subred de red puente inactiva con el Hotspot móvil:** El iPhone Hotspot configuró la subred física `172.20.10.0/28`. En las máquinas de los workers existían redes puente viejas de Docker en `172.20.0.0/16` (`br-XXXX`), por lo que los paquetes destinados a `172.20.10.2` (Maestro) eran absorbidos por el puente inactivo con error `Destination Host Unreachable`. Se solucionó ejecutando `docker network prune -f` en las máquinas de los workers.
3. **Colisión de Hostnames (`docker-desktop`):** Todos los contenedores iniciaban con el mismo hostname `docker-desktop`. OpenMPI confundía los procesos remotos con procesos locales en el mismo nodo. Se solucionó configurando un mapeo estricto en `/etc/hosts` (`maestro`, `cami`, `marines`).
4. **Suspensión de red en reposo del Hotspot:** Al apagarse la pantalla del teléfono celular, el enlace Wi-Fi suspendía el paso de paquetes TCP arrojando `No route to host`, requiriendo mantener el dispositivo móvil desbloqueado y activo.

**3. Tiempo local (T_s) vs tiempo en el clúster físico:**

| Configuración | Nodos | Procesos ($N$) | Tiempo ($T$) | Speedup ($S$) | Eficiencia ($E$) |
|---|:---:|:---:|:---:|:---:|:---:|
| **Local (Línea Base)** | 1 (Laptop Carlos) | 1 | **42.905149 s** | 1.00x | 100.0% |
| **Clúster Físico Distribuido** | 3 Laptops (Wi-Fi) | 12 | **50.729352 s** | **0.8458x** (~0.85x) | **7.05%** |

**Análisis del resultado:**
A pesar de contar con 12 núcleos físicos distribuidos en 3 computadoras, el tiempo no disminuyó, sino que aumentó ligeramente de 42.90 s a 50.73 s (*Speedup* ~0.85x). La razón fundamental de este comportamiento radica en la **estrategia de partición del espacio de búsqueda**:
El algoritmo implementa una división estática en bloques contiguos (`upper / N`). La llave secreta del Grupo 9 (`3141592`) es un valor pequeño que cae dentro del primer bloque $[0, 6 × 10¹⁵]$, el cual fue asignado exclusivamente al **Proceso 0** (nodo Maestro). Por lo tanto, el Proceso 0 tuvo que recorrer exactamente las mismas 3,141,592 llaves de manera secuencial que en la prueba mononúcleo local. Los otros 11 procesos estuvieron explorando rangos superiores sin aportar al hallazgo de esta llave específica. La leve diferencia de tiempo adicional se atribuye al *overhead* de la virtualización de Docker, la contención en el bus de memoria al ejecutar 4 procesos concurrentes en la misma máquina física y el costo de inicialización y sincronización de los demonios de red.

**4. ¿Qué pasa si configuran el hostfile para 32 procesos MPI en una máquina con solo 4 núcleos físicos?**

Ocurre una degradación drástica del rendimiento por **sobreasignación (*oversubscription*)**. Cuando el número de procesos supera con creces el número de núcleos físicos de CPU, el planificador del sistema operativo (*scheduler*) debe realizar constantes **cambios de contexto (*context switching*)** para dar tiempo de procesador a cada uno de los 32 procesos. 
En un algoritmo intensivo en cómputo como DES, cambiar de contexto obliga a guardar y restaurar constantemente los registros del procesador y causa una invalidación masiva de las memorias caché (L1 y L2), forzando a la CPU a recargar los datos desde la memoria principal (fenómeno conocido como *cache thrashing*). En lugar de acelerar el cálculo, la computadora desperdicia la mayor parte de sus ciclos de reloj administrando la concurrencia en vez de ejecutar instrucciones de cifrado, multiplicando el tiempo total de ejecución.

**5. Vulnerabilidades y retos logísticos de administrar HPC con hardware heterogéneo interconectado:**

- **Vulnerabilidades de seguridad:**
  1. *Acceso total sin contraseñas:* La configuración requerida de `authorized_keys` con cuentas de `root` y la directiva `OMPI_ALLOW_RUN_AS_ROOT=1` otorga acceso de superusuario irrestricto entre todos los nodos del clúster; si un atacante compromete una laptop en la red local, tiene control total inmediato de todo el clúster.
  2. *Tráfico en texto plano:* Las comunicaciones internas de OpenMPI no están cifradas por defecto (para optimizar la latencia), por lo que cualquier dispositivo conectado a la misma red Wi-Fi podría capturar los datos y fragmentos de mensajes mediante un ataque de escucha (*sniffing* o ARP spoofing).
- **Retos logísticos:**
  1. *Efecto del eslabón más débil (Stragglers):* Si una máquina del clúster tiene un procesador más lento, poca memoria RAM o sufre de estrangulamiento térmico (*thermal throttling* en laptops a batería), todas las máquinas más rápidas se ven obligadas a esperar en las barreras colectivas de MPI, degradando la eficiencia global al ritmo del nodo más lento.
  2. *Heterogeneidad de software y red:* Mantener versiones perfectamente alineadas de bibliotecas (OpenSSL, OpenMPI, compiladores) y sortear configuraciones dispares de cortafuegos (*firewalls* de Windows, macOS y Linux) requiere herramientas de orquestación complejas como Docker, las cuales introducen sus propias capas de abstracción y complejidades de red.

**6. ¿Lograron el objetivo? ¿Qué parte les gustó más / menos? ¿Qué habrían hecho mejor?**

- **¿Lograron el objetivo?** Sí, en su totalidad. Se interconectaron exitosamente 3 computadoras físicas mediante contenedores Docker en un clúster de 12 núcleos sobre una red Wi-Fi real, se descifró la clave secreta `3141592` del Grupo 9, se recuperó el texto original completo de 352 palabras y se midieron las métricas de rendimiento reales del clúster.
- **Lo que más nos gustó:** La experiencia práctica de orquestar un clúster de supercómputo real distribuido entre laptops físicas, y la satisfacción de diagnosticar y resolver a nivel de ingeniería de redes los problemas de enrutamiento TCP, colisión de subredes y barreras de Docker.
- **Lo que menos nos gustó:** Las particularidades y limitaciones del motor de red de Docker Desktop en macOS, que al estar intermediado por una máquina virtual oculta las interfaces físicas y complica la conectividad directa requerida por protocolos HPC como MPI.
- **¿Qué habríamos hecho mejor?** Diseñar un esquema de **balanceo dinámico de carga** (modelo Maestro-Trabajador con asignación de pequeños bloques de trabajo de $10^5$ llaves bajo demanda) en lugar de una partición estática en bloques grandes. Esto habría permitido que todos los 12 núcleos contribuyeran efectivamente a la búsqueda al inicio del espacio de llaves, logrando un *speedup* real cercano a 10x-11x.
