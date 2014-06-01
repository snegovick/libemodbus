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
  char *buf = ":0603006B0003AB\r\ngjksdfg:0603006b0003AB\r\nsdghsdf\0";
  uint8_t storage[32];
  struct emb e;
  emb_init(&e, storage, 32, process);
  int i = 0;
  for (;i<49;i++) {
    printf("in: %c\r\n", buf[i]);
    emb_process_data(&e, buf+i, 1);
  }
  return 0;
}
