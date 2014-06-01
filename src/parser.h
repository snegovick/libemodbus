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
  unsigned int parse_buffer_offset;
  void (*process)(const void *data, unsigned int size);
  int condition;
};

int emb_init(struct emb *e, uint8_t *storage, unsigned int storage_size, void (*process)(const void *data, unsigned int size)) {
  cb_init(&e->cb, storage, storage_size);
  e->process = process;
  e->condition = COND_START;
  e->parse_buffer_offset = 0;
  return CERR_OK;
}

int __emb_parse_incoming_data(struct emb *e) {
  printf("processing new data\r\n");
  e->process(e->parse_buffer, e->parse_buffer_offset);
  return CERR_OK;
}

void emb_recover(struct emb *e) {
  e->condition = COND_START;
  cb_recover(&(e->cb));
}

int emb_push_data(struct emb *e, const uint8_t *new_data, unsigned int data_size) {
  return cb_push(&(e->cb), new_data, data_size);
}

int emb_process_data(struct emb *e) {
  int ret = CERR_OK;
  unsigned int i = 0;
  uint8_t c;
  while (cb_get_allocated_space(&(e->cb))>0) {
    cb_pop(&(e->cb), &c, 1);
    if ((e->condition == COND_START) && (c==':')) {
      e->parse_buffer_offset = 0;
      e->parse_buffer[e->parse_buffer_offset] = c;
      e->parse_buffer_offset ++;
      e->condition = COND_R;
    } else if (e->condition == COND_R) {
      e->parse_buffer[e->parse_buffer_offset] = c;
      e->parse_buffer_offset ++;
      if (c=='\r') {
        e->condition = COND_N;
      }
    } else if ((e->condition == COND_N)) {
      if (c != '\n') {
        emb_recover(e);
      } else {
        e->parse_buffer[e->parse_buffer_offset] = c;
        e->parse_buffer_offset ++;
        ret = __emb_parse_incoming_data(e);
        e->condition = COND_START;
      }
    }

    if (e->parse_buffer_offset >= STATIC_BUFFER_SIZE) {
      e->condition = COND_START;
      return CERR_ENOMEM;
    }
  }
  return ret;
}

#endif/*__PARSER_H__*/
