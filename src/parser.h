#ifndef __PARSER_H__
#define __PARSER_H__

#include <circularbuf.h>
#include <cdynstorage.h>

#define COND_START 0
#define COND_R 1
#define COND_N 2

#define STATIC_BUFFER_SIZE 128

struct emb {
  struct circular_buffer cb;
  uint8_t parse_buffer[STATIC_BUFFER_SIZE];
  void (*process)(const void *data, unsigned int size);
  int condition;
  unsigned int start;
  unsigned int end;
};

int emb_init(struct emb *e, const uint8_t *storage, unsigned int storage_size, void (*process)(const void *data, unsigned int size)) {
  cb_init(&e->cb, storage, storage_size);
  e->process = process;
  return CERR_OK;
}

int emb_push_data(struct emb *e, const uint8_t *new_data, unsigned int data_size) {
  cb_push(&e->cb, new_data, data_size);
  unsigned int i = 0;
  for (;i<data_size;i++) {
    cb_push_byte(&e->cb, new_data[i]);
    if ((e->condition == COND_START) && (new_data[i] == ':')) {
      e->condition = COND_R;
      e->start = cb_get_current_position(&e->cb);
    } else if ((e->condition == COND_R) && (new_data[i] == '\r')) {
      e->condition = COND_N;
    } else if ((e->condition == COND_N) && (new_data[i] == '\n')) {
      e->condition = COND_START;
      e->end = cb_get_current_position(&e->cb);
      if (cb_get_data(&e->cb, e->start, e->end, e->parse_buffer, STATIC_BUFFER_SIZE)>0) {
        
      }
    }
  }
}

#endif/*__PARSER_H__*/
