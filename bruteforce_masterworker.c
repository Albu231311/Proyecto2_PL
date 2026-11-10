
#include <ctype.h>
#include <mpi.h>
#include <openssl/des.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TAG_REQUEST 1
#define TAG_CHUNK 2
#define TAG_FOUND 3
#define TAG_STOP 4

static long CHUNK_SIZE =
    1000000; /* llaves por chunk; ajustable por env CHUNK_SIZE */

/* ---- Mismas funciones DES que bruteforce.c (sin cambios) ---- */

static void des_ecb(long key, char *ciph, int len, int enc) {
  unsigned long kk = (unsigned long)key;
  unsigned long k = 0;
  for (int i = 0; i < 8; ++i) {
    kk <<= 1;
    k += (kk & (0xFEUL << i * 8));
  }
  DES_cblock kb;
  memcpy(kb, &k, 8);
  DES_set_odd_parity(&kb);
  DES_key_schedule ks;
  DES_set_key_unchecked((const_DES_cblock *)&kb, &ks);
  for (int off = 0; off < len; off += 8) {
    DES_ecb_encrypt((const_DES_cblock *)(ciph + off),
                    (DES_cblock *)(ciph + off), &ks, enc);
  }
}

void my_decrypt(long key, char *ciph, int len) {
  des_ecb(key, ciph, len, DES_DECRYPT);
}
void my_encrypt(long key, char *ciph, int len) {
  des_ecb(key, ciph, len, DES_ENCRYPT);
}

const char *search = "dado";
int tryKey(long key, char *ciph, int len) {
  char temp[len + 1];
  memcpy(temp, ciph, len);
  temp[len] = 0;
  my_decrypt(key, temp, len);
  return strstr(temp, search) != NULL;
}

int count_words(const char *t) {
  int words = 0, in_word = 0;
  for (; *t; t++) {
    if (isspace((unsigned char)*t))
      in_word = 0;
    else if (!in_word) {
      in_word = 1;
      words++;
    }
  }
  return words;
}

char eltexto[] = "Ya le he dado tres vueltas al mundo\n"
                 "Tratando de olvidar (tratando de olvidar)\n"
                 "Y pinté besos sobre los tuyos\n"
                 "No los logré tapar (no los logré tapar)\n"
                 "Dime qué hago si estás en todos lados\n"
                 "Tú eres ese clavo que no logré sacar\n"
                 "Me voy de viaje y termino por tu calle\n"
                 "Es que mis pies nos saben llegar a otro lugar\n"
                 "Dicen que todos los caminos\n"
                 "Nos llevan a un mismo destino\n"
                 "Llevan a Roma, llevan a Roma\n"
                 "Pero por más que me desvío\n"
                 "Sé que el final de mi camino\n"
                 "No llega a Roma, me lleva hasta a ti\n"
                 "Ya te he ahogado en millones de copas\n"
                 "Siempre sales a flotar\n"
                 "Me he tatuado los nombres de otras\n"
                 "Pero el tuyo duele más\n"
                 "Dime qué hago si estás en todos lados\n"
                 "Tú eres ese clavo que no logré sacar\n"
                 "Me voy de viaje y termino por tu calle\n"
                 "Es que mis pies nos saben llegar a otro lugar\n"
                 "Dicen que todos los caminos\n"
                 "Nos llevan a un mismo destino\n"
                 "Llevan a Roma, llevan a Roma\n"
                 "Pero por más que me desvío\n"
                 "Sé que el final de mi camino\n"
                 "No llega a Roma, me lleva hasta a ti\n"
                 "A ti, a ti, a ti\n"
                 "A ti, a ti, a ti\n"
                 "No llega a Roma, me lleva hasta a ti\n"
                 "A ti, a ti, a ti\n"
                 "A ti, a ti, a ti\n"
                 "No llega a Roma, me lleva hasta a ti\n"
                 "Dime qué hago si estás en todos lados\n"
                 "Tú eres ese clavo que no logré sacar\n"
                 "Me voy de viaje y termino por tu calle\n"
                 "Es que mis pies nos saben llegar a otro lugar\n"
                 "Dicen que todos los caminos\n"
                 "Nos llevan a un mismo destino\n"
                 "Llevan a Roma, llevan a Roma\n"
                 "Pero por más que me desvío\n"
                 "Sé que el final de mi camino\n"
                 "No llega a Roma, me lleva hasta a ti\n"
                 "A ti, a ti, a ti\n"
                 "A ti, a ti, a ti (no llega)\n"
                 "No llega a Roma, me lleva hasta a ti\n"
                 "A ti, a ti, a ti\n"
                 "A ti, a ti, a ti (me lleva)\n"
                 "No llega a Roma, me lleva hasta a ti";
long the_key = 30000000; /* (0 a 2^56 - 1) */

/* ---- Master: administra el espacio de llaves, no busca ----
 *
 * Invariante clave (evita el bug de "mensajes sin emparejar" en
 * MPI_Finalize): el master SIEMPRE responde exactamente una vez a cada
 * mensaje que recibe -- ya sea TAG_REQUEST (con un chunk o un STOP) o
 * TAG_FOUND (con un STOP) -- y solo deja de escuchar cuando ha mandado
 * un STOP por cada uno de los N-1 workers. Antes mandaba un "blast" de
 * STOP a todos apenas llegaba el FOUND y salia del loop de inmediato;
 * eso dejaba sin responder cualquier TAG_REQUEST que un worker mandara
 * DESPUES de ese blast (p.ej. uno que seguia ocupado con su chunk
 * anterior), y ese mensaje huerfano era justo el error en Finalize. */
static void run_master(int N, MPI_Comm comm) {
  const long UPPER = (1L << 56);
  long next_key = 0;
  long found_key = -1;
  int found = 0;
  int workers_active = N - 1;
  long chunks_sent = 0;

  double t_start = MPI_Wtime();

  while (workers_active > 0) {
    MPI_Status st;
    long msg[1] = {0};
    /* Mensajes de los workers: TAG_REQUEST (vacio) o TAG_FOUND (1 long) */
    MPI_Recv(msg, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &st);
    int worker = st.MPI_SOURCE;

    if (st.MPI_TAG == TAG_FOUND) {
      if (!found) {
        found = 1;
        found_key = msg[0];
        printf("[Master] Worker %d encontro la llave: %ld\n", worker,
               found_key);
      }
      MPI_Send(NULL, 0, MPI_BYTE, worker, TAG_STOP, comm);
      workers_active--;
    } else { /* TAG_REQUEST */
      if (found || next_key >= UPPER) {
        MPI_Send(NULL, 0, MPI_BYTE, worker, TAG_STOP, comm);
        workers_active--;
      } else {
        long chunk[2];
        chunk[0] = next_key;
        chunk[1] =
            (next_key + CHUNK_SIZE < UPPER) ? next_key + CHUNK_SIZE : UPPER;
        next_key = chunk[1];
        chunks_sent++;
        MPI_Send(chunk, 2, MPI_LONG, worker, TAG_CHUNK, comm);
      }
    }
  }

  double t_end = MPI_Wtime();
  printf("[Master] Chunks repartidos: %ld (tamano=%ld llaves c/u)\n",
         chunks_sent, CHUNK_SIZE);
  printf("[Master] Tiempo total: %.6f segundos\n", t_end - t_start);

  if (found) {
    /* Reenviar la llave encontrada a todos para que rank 0 la pueda
     * imprimir y descifrar -- el master no participa de la busqueda,
     * asi que no tiene el texto descifrado todavia (eso se maneja en
     * main() despues de llamar a run_master). */
    extern long g_found_key;
    g_found_key = found_key;
  }
}

/* ---- Worker: pide chunks, los procesa, repite hasta TAG_STOP ---- */
static long run_worker(int id, char *cipher, int ciphlen, MPI_Comm comm) {
  long found_key = -1;
  long chunks_done = 0;
  long keys_tried = 0;

  while (1) {
    MPI_Send(NULL, 0, MPI_BYTE, 0, TAG_REQUEST, comm);

    MPI_Status st;
    long chunk[2];
    MPI_Recv(chunk, 2, MPI_LONG, 0, MPI_ANY_TAG, comm, &st);

    if (st.MPI_TAG == TAG_STOP)
      break;

    chunks_done++;
    for (long k = chunk[0]; k < chunk[1]; k++) {
      keys_tried++;
      if (tryKey(k, cipher, ciphlen)) {
        found_key = k;
        long msg[1] = {k};
        MPI_Send(msg, 1, MPI_LONG, 0, TAG_FOUND, comm);
        /* Esperar el STOP del master para salir ordenado (el master
         * siempre responde a este mensaje con exactamente un TAG_STOP) */
        MPI_Status st2;
        MPI_Recv(NULL, 0, MPI_BYTE, 0, TAG_STOP, comm, &st2);
        printf("[Worker %d] encontro la llave %ld (probo %ld llaves, %ld "
               "chunks)\n",
               id, k, keys_tried, chunks_done);
        return found_key;
      }
    }
  }

  printf(
      "[Worker %d] termino sin encontrar nada (probo %ld llaves, %ld chunks)\n",
      id, keys_tried, chunks_done);
  return -1;
}

long g_found_key = -1;

int main(int argc, char *argv[]) {
  int N, id;
  MPI_Status st;
  (void)st;

  if (getenv("CHUNK_SIZE")) {
    CHUNK_SIZE = atol(getenv("CHUNK_SIZE"));
  }

  int file_mode = (argc >= 3);
  char *cipher = NULL;
  int ciphlen = 0;
  int pad = 0;

  if (file_mode) {
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
      fprintf(stderr, "No pude abrir el archivo cifrado: %s\n", argv[1]);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    cipher = malloc(fsize + 1);
    if (fread(cipher, 1, fsize, f) != (size_t)fsize) {
      fprintf(stderr, "Error leyendo %s\n", argv[1]);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    fclose(f);
    ciphlen = (int)fsize;
    cipher[ciphlen] = 0;
    search = argv[2];
  } else {
    if (argc > 1)
      the_key = atol(argv[1]);
    int textlen = strlen(eltexto);
    pad = 8 - (textlen % 8);
    ciphlen = textlen + pad;
    cipher = malloc(ciphlen + 1);
    memcpy(cipher, eltexto, textlen);
    memset(cipher + textlen, pad, pad);
    cipher[ciphlen] = 0;
    my_encrypt(the_key, cipher, ciphlen);
  }

  MPI_Init(&argc, &argv);
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  if (N < 2) {
    if (id == 0)
      fprintf(stderr,
              "Se necesitan al menos 2 procesos (1 master + >=1 worker)\n");
    MPI_Abort(comm, 1);
  }

  if (id == 0) {
    if (file_mode) {
      printf("Modo archivo externo: %s (%d bytes), palabra objetivo = \"%s\"\n",
             argv[1], ciphlen, search);
    } else {
      printf("Texto: %d palabras, %d bytes (%d bytes cifrados)\n",
             count_words(eltexto), ciphlen - pad, ciphlen);
      printf("Buscando llave privada: %ld\n", the_key);
    }
    printf("Nodo 0 = MASTER (no busca, solo reparte chunks de %ld llaves)\n",
           CHUNK_SIZE);
    printf("Nodos 1..%d = WORKERS (%d en total)\n", N - 1, N - 1);
  }

  /* MPI_Barrier removido: en Master-Worker puro los workers solo se comunican
   * con rank 0 punto a punto; remover el barrier evita conexiones all-to-all
   * entre workers de diferentes subredes/contenedores */
  /* MPI_Barrier(comm); */

  if (id == 0) {
    run_master(N, comm);
  } else {
    run_worker(id, cipher, ciphlen, comm);
  }

  /* MPI_Barrier(comm); */

  if (id == 0 && g_found_key >= 0) {
    my_decrypt(g_found_key, cipher, ciphlen);
    cipher[ciphlen - pad] = 0;
    printf("\nKey = %ld\n", g_found_key);
    printf("%s\n", cipher);
  } else if (id == 0) {
    printf("No se encontro la llave en el espacio buscado.\n");
  }

  free(cipher);
  MPI_Finalize();
  return 0;
}
