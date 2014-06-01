#ifndef __PARSER_H__
#define __PARSER_H__

#include <circularbuf.h>

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

int emb_init(struct emb *e, uint8_t *storage, unsigned int storage_size, void (*process)(const void *data, unsigned int size)) {
  cb_init(&e->cb, storage, storage_size);
  e->process = process;
  e->condition = COND_START;
  return CERR_OK;
}

int __emb_parse_incoming_data(struct emb *e) {
}

int emb_process_data(struct emb *e, const uint8_t *new_data, unsigned int data_size) {
  unsigned int i = 0;
  for (;i<data_size;i++) {
    printf("pushing %c\r\n", new_data[i]);
    if ((e->condition == COND_START) && (new_data[i] == ':')) {
      e->condition = COND_R;
      e->start = cb_get_current_position(&(e->cb));
      cb_push_byte(&(e->cb), new_data[i]);
    } else if ((e->condition == COND_R) && (new_data[i] == '\r')) {
      cb_push_byte(&(e->cb), new_data[i]);
      e->condition = COND_N;
    } else if ((e->condition == COND_N) && (new_data[i] == '\n')) {
      e->condition = COND_START;
      cb_push_byte(&(e->cb), new_data[i]);
      e->end = cb_get_current_position(&e->cb);

      unsigned int acq_size = 0;
      if (cb_get_data(&e->cb, e->start, e->end, e->parse_buffer, STATIC_BUFFER_SIZE, &acq_size)==CERR_OK) {
        e->process(e->parse_buffer, acq_size);
      }

    } else {
      cb_push_byte(&(e->cb), new_data[i]);
    }
  }
}

#endif/*__PARSER_H__*/
