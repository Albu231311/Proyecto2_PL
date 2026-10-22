#include <ctype.h>
#include <mpi.h>
#include <openssl/des.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/* Antes: array fijo "dado" + tryKey solo miraba los primeros 16 bytes.
 * Eso funcionaba para la autoprueba porque "dado" cae justo ahi, pero
 * para la Fase 2 la palabra objetivo la da OTRO equipo y puede caer en
 * cualquier parte de su texto -- por eso ahora `search` es un puntero
 * reasignable y tryKey descifra/busca en el buffer COMPLETO (`len`). */
const char *search = "dado";
int tryKey(long key, char *ciph, int len) {
  char temp[len + 1];
  memcpy(temp, ciph, len);
  temp[len] = 0;
  my_decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

int count_words(const char *t) {
  int words = 0, in_word = 0;
  for (; *t; t++) {
    if (isspace((unsigned char)*t)) {
      in_word = 0;
    } else if (!in_word) {
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

int main(int argc, char *argv[]) {
  int N, id;
  long upper = (1L << 56);
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;

  /* Modo Fase 2 (archivo externo de otro equipo):
   *   ./bruteforce <archivo_cifrado> "<palabra objetivo>"
   * Modo Fase 1 (autoprueba, como antes):
   *   ./bruteforce              -> autocifra eltexto con the_key por defecto
   *   ./bruteforce <llave>      -> autocifra eltexto con esa llave (tabla de tiempos) */
  int file_mode = (argc >= 3);

  char *cipher;
  int ciphlen;
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
    search = argv[2]; /* palabra objetivo anunciada por el otro equipo */
  } else {
    if (argc > 1) {
      the_key = atol(argv[1]);
    }
    int textlen = strlen(eltexto);
    pad = 8 - (textlen % 8);
    ciphlen = textlen + pad;
    cipher = malloc(ciphlen + 1);
    memcpy(cipher, eltexto, textlen);
    memset(cipher + textlen, pad, pad);
    cipher[ciphlen] = 0;
    my_encrypt(the_key, cipher, ciphlen); /* CAMBIO 3 */
  }

  MPI_Init(&argc, &argv);
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  long found = 0;
  int ready = 0;

  if (id == 0) {
    if (file_mode) {
      printf("Modo archivo externo: %s (%d bytes), palabra objetivo = \"%s\"\n",
             argv[1], ciphlen, search);
    } else {
      printf("Texto: %d palabras, %d bytes (%d bytes cifrados)\n",
             count_words(eltexto), ciphlen - pad, ciphlen);
      printf("Buscando llave privada: %ld\n", the_key);
    }
    printf("Primeros 16 bytes cifrados:");
    for (int i = 0; i < 16 && i < ciphlen; i++)
      printf(" %02x", (unsigned char)cipher[i]);
    printf("\n");
  }

  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id + 1) - 1;
  if (id == N - 1) {
    myupper = upper;
  }
  printf("Process %d lower %ld upper %ld\n", id, mylower, myupper);

  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  MPI_Barrier(comm);
  double t_start = MPI_Wtime();

  for (long i = mylower; i < myupper; ++i) {
    MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
    if (ready)
      break;

    if (tryKey(i, cipher, ciphlen)) {
      found = i;
      printf("Process %d found the key\n", id);
      for (int node = 0; node < N; node++) {
        MPI_Send(&found, 1, MPI_LONG, node, 0, comm);
      }
      break;
    }
  }

  MPI_Wait(&req, &st);
  MPI_Barrier(comm);
  double t_end = MPI_Wtime();
  double elapsed = t_end - t_start;

  if (id == 0) {
    my_decrypt(found, cipher, ciphlen);
    cipher[ciphlen - pad] = 0;
    printf("Key = %li\n", found);
    printf("Tiempo de busqueda: %.6f segundos\n\n", elapsed);
    printf("%s\n", cipher);
  }

  MPI_Finalize();
}