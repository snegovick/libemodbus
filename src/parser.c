#include "parser.h"
#include <stdio.h>

void process(const void * data, unsigned int size) {
  int i = 0;
  char * data_ptr = (char*)data;
  printf("\r\nprocess data: ");
  for (;i<size;i++) {
    printf("%c", data_ptr[i]);
  }
}

int main (void) {
  char *buf = ":0603006Baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0003AB\r\ngjksdfg:0603006b0003AB\r\nsdghsdf";
  uint8_t storage[32];
  struct emb e;
  emb_init(&e, storage, 32, process);
  int i = 1;
  char *c = buf;
  while (1) {
    printf("in: %c\r\n", *c);
    emb_push_data(&e, c, 1);
    i++;
    if (i%10==0) {
      if (emb_process_data(&e)!=CERR_OK) {
        printf ("error\r\n");
      }
    }
    c++;
    if (*c=='\0') {
      break;
    }
  }
  if (emb_process_data(&e)!=CERR_OK) {
    printf ("error\r\n");
  }

  return 0;
}
