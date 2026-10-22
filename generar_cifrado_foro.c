/* Genera el archivo binario cifrado para publicar en el foro de Canvas.
 * Usa exactamente el mismo cifrado (des_ecb + padding) que bruteforce.c,
 * para que cualquier equipo que lo descargue pueda atacarlo con:
 *   ./bruteforce roma_cifrado.bin "tatuado"
 *
 * La llave privada (the_key) NO se publica -- es justo lo que el otro
 * equipo tiene que encontrar.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/des.h>

static void des_ecb(long key, char *ciph, int len, int enc) {
  unsigned long kk = (unsigned long)key, k = 0;
  for (int i = 0; i < 8; ++i) { kk <<= 1; k += (kk & (0xFEUL << i * 8)); }
  DES_cblock kb;
  memcpy(kb, &k, 8);
  DES_set_odd_parity(&kb);
  DES_key_schedule ks;
  DES_set_key_unchecked((const_DES_cblock *)&kb, &ks);
  for (int off = 0; off < len; off += 8)
    DES_ecb_encrypt((const_DES_cblock *)(ciph + off), (DES_cblock *)(ciph + off), &ks, enc);
}

/* Mismo texto que eltexto[] en bruteforce.c */
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

int main(int argc, char *argv[]) {
  long the_key = 30000000; /* misma llave privada por defecto de bruteforce.c */
  if (argc > 1) the_key = atol(argv[1]);

  int textlen = strlen(eltexto);
  int pad = 8 - (textlen % 8);
  int ciphlen = textlen + pad;
  char *buf = malloc(ciphlen);
  memcpy(buf, eltexto, textlen);
  memset(buf + textlen, pad, pad);
  des_ecb(the_key, buf, ciphlen, DES_ENCRYPT);

  FILE *f = fopen("roma_cifrado.bin", "wb");
  fwrite(buf, 1, ciphlen, f);
  fclose(f);

  printf("roma_cifrado.bin generado: %d bytes (texto=%d + padding=%d)\n", ciphlen, textlen, pad);
  printf("Llave privada usada (NO publicar): %ld\n", the_key);
  printf("Palabra objetivo a anunciar en el foro: tatuado\n");
  return 0;
}
