#ifndef __PARSER_H__
#define __PARSER_H__

#include <circularbuf.h>

#define COND_START 0
#define COND_R 1
#define COND_N 2

#define STATIC_BUFFER_SIZE 128
#define BINARY_BUFFER_SIZE 128


#define F_READ_COIL_STATUS          1
#define F_READ_INPUT_STATUS         2
#define F_READ_HOLDING_REGISTERS    3
#define F_READ_INPUT_REGISTERS      4
#define F_FORCE_SINGLE_COIL         5
#define F_PRESET_SINGLE_REGISTER    6
#define F_READ_EXCEPTION_STATUS     7
#define F_DIAGNOSTICS               8
#define F_FORCE_MULTIPLE_COILS      15
#define F_PRESET_MULTIPLE_REGISTERS 16
#define F_REPORT_SLAVE_ID           17

struct query_header {
  uint8_t header;
  uint8_t slave_address;
  uint8_t function;
};

struct read_coil_status_q {
  uint16_t starting_address;
  uint16_t number_of_points;
};

struct read_coil_status_r {
  uint8_t byte_count;
  uint8_t *data;
};

#define read_input_status_q read_coil_status_q
#define read_input_status_r read_coil_status_r

#define read_holding_registers_q read_coil_status_q

struct read_holding_registers_r {
  uint8_t byte_count;
  uint16_t *data;
};

#define read_input_registers_q read_coil_status_q
#define read_input_registers_r read_holding_registers_r

struct force_single_coil_q {
  uint16_t address;
  uint16_t data;
};

struct force_single_coil_r {
  uint16_t address;
  uint16_t data;
};

#define preset_single_register_q force_single_coil_q
#define preset_single_register_r force_single_coil_r

struct query {
  struct query_header header;
  union {
    struct read_coil_status_q rcsq;
    struct force_single_coil_q fscq;
  };
};

struct response {
  struct query_header header;
  union {
    struct read_coil_status_r rcsr;
    struct read_holding_registers_r rhrr;
    struct force_single_coil_r fscr;
  };
};


struct emb {
  struct circular_buffer cb;
  uint8_t parse_buffer[STATIC_BUFFER_SIZE];
  unsigned int parse_buffer_offset;
  uint8_t binary_data[BINARY_BUFFER_SIZE];
  void (*process_query)(const void *data, unsigned int size, struct query *qs);
  void (*process_response)(const void *data, unsigned int size, struct response *rs);
  int condition;
  int master;
};


uint8_t __hex_to_byte(uint8_t hi, uint8_t lo) {
  uint8_t byte = 0;
  uint8_t b = hi;
  if (b>='A' && b<='F') {
    byte = (uint8_t)(b-'A');
  } else if (b>='a' && b<='f') {
    byte = (uint8_t)(b-'a');
  } else if (b>='0' && b<='9') {
    byte = (uint8_t)b-'0';
  }
  byte<<4;
  b = lo;
  if (b>='A' && b<='F') {
    byte |= (uint8_t)(b-'A');
  } else if (b>='a' && b<='f') {
    byte |= (uint8_t)(b-'a');
  } else if (b>='0' && b<='9') {
    byte |= (uint8_t)b-'0';
  }
  return byte;
}

uint16_t __hex_to_short(uint8_t hi0, uint8_t lo0, uint8_t hi1, uint8_t lo1) {
  uint8_t hi = __hex_to_byte(hi0, lo0);
  uint8_t lo = __hex_to_byte(hi1, lo1);
  return (hi<<8 | lo);
}

int emb_init(struct emb *e, uint8_t *storage, unsigned int storage_size, void (*process_response)(const void *data, unsigned int size, struct response *rs), void (*process_query)(const void *data, unsigned int size, struct query *qs), int master) {
  cb_init(&e->cb, storage, storage_size);
  e->process_response = process_response;
  e->process_query = process_query;
  e->condition = COND_START;
  e->parse_buffer_offset = 0;
  e->master = master;
  return CERR_OK;
}

int __emb_parse_header(struct emb *e, struct query_header *qh, unsigned int *offset) {
  int ret = CERR_OK;
  uint8_t *pb = e->parse_buffer;
  if (pb[0] == ':') {
    qh->header = ':';
  } else {
    return CERR_ENOTFOUND;
  }
  qh->slave_address = __hex_to_byte(pb[1], pb[2]);
  qh->function = __hex_to_byte(pb[3], pb[4]);
  *offset = 5;
  return ret;
}

int __emb_read_coil_status(struct emb *e, struct read_coil_status_r *rcsr, unsigned int *offset) {
  int ret = CERR_OK;
  uint8_t *pb = e->parse_buffer;
  rcsr->byte_count = __hex_to_byte(pb[*offset], pb[*offset+1]);
  *offset+=2;
  unsigned int i = 0;
  if (rcsr->byte_count > BINARY_BUFFER_SIZE) {
    return CERR_ENOMEM;
  }
  for (;i<rcsr->byte_count;i++) {
    rcsr->data[i] = __hex_to_byte(pb[*offset], pb[*offset+1]);
    *offset+=2;
  }
  return ret;
}

int __emb_force_single_coil(struct emb *e, struct force_single_coil_r *fscr, unsigned int *offset) {
  int ret = CERR_OK;
  uint8_t *pb = e->parse_buffer;

  rcsr->address = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  *offset+=4;
  rcsr->data = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  *offset+=4;
  return ret;
}


int __emb_parse_incoming_data(struct emb *e) {
  printf("processing new data\r\n");
  struct query_header qh;
  unsigned int offset = 0;
  struct response rs;
  if (e->master) {
    // if im master, then responses are incoming
    __emb_parse_header(e, &qh, &offset);
    memcpy(&(rs.header), &qh, sizeof(struct query_header));
    switch (qh.function) {
    case F_READ_COIL_STATUS:
    case F_READ_INPUT_STATUS:
    case F_READ_INPUT_REGISTERS: {
      rs.rcsr.data = e->binary_data;
      __emb_read_coil_status(e, &(rs.rcsr), &offset);
      break;
    }
    case F_FORCE_SINGLE_COIL:
    case F_PRESET_SINGLE_REGISTER: {
      __emb_force_single_coil(e, &(rs.fscr), &offset);
      break;
    }
    }
  }
  e->process_response(e->parse_buffer, e->parse_buffer_offset, &(rs));
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
