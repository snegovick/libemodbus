#include "emodbus_parser.h"

#define CONTAINER_OVERHEAD 5

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
  byte<<=4;
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

uint8_t hex_values[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void __byte_to_hex(uint8_t byte, uint8_t *hi, uint8_t *lo) {
  *hi = hex_values[byte>>4];
  *lo = hex_values[byte&0x0F];
}

int emb_init(struct emb *e, uint8_t *storage, unsigned int storage_size, void (*process_response)(const void *data, unsigned int size, struct response *rs), void (*process_query)(const void *data, unsigned int size, struct query *qs), int master, uint8_t address) {
  cb_init(&e->cb, storage, storage_size);
  e->address = address;
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
  if (pb[0] != ':') {
    return CERR_ENOTFOUND;
  }
  qh->slave_address = __hex_to_byte(pb[1], pb[2]);
  qh->function = __hex_to_byte(pb[3], pb[4]);
  *offset = 5;
  return ret;
}

int __emb_read_coil_status_r(struct emb *e, struct read_coil_status_r *rcsr, unsigned int *offset) {
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

int __emb_force_single_coil_r(struct emb *e, struct force_single_coil_r *fscr, unsigned int *offset) {
  int ret = CERR_OK;
  uint8_t *pb = e->parse_buffer;

  fscr->address = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  *offset+=4;
  fscr->data = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  *offset+=4;
  return ret;
}

int __emb_read_coil_status_q(struct emb *e, struct read_coil_status_q *rcsq, unsigned int *offset) {
  int ret = CERR_OK;
  uint8_t *pb = e->parse_buffer;
  rcsq->starting_address = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  rcsq->number_of_points = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  *offset+=4;
  return ret;
}

int __emb_force_single_coil_q(struct emb *e, struct force_single_coil_q *fscq, unsigned int *offset) {
  int ret = CERR_OK;
  uint8_t *pb = e->parse_buffer;

  fscq->address = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  *offset+=4;
  fscq->data = __hex_to_short(pb[*offset], pb[*offset+1], pb[*offset+2], pb[*offset+3]);
  *offset+=4;
  return ret;
}


int __emb_parse_incoming_data(struct emb *e) {
  struct query_header qh;
  unsigned int offset = 0;
  struct response rs;
  struct query qs;

  __emb_parse_header(e, &qh, &offset);

  if (e->master) {
    memcpy(&(rs.header), &qh, sizeof(struct query_header));
    /* if im master, then responses are incoming */
    switch (qh.function) {
    case F_READ_COIL_STATUS:
    case F_READ_INPUT_STATUS:
    case F_READ_INPUT_REGISTERS: {
      rs.rcsr.data = e->binary_data;
      __emb_read_coil_status_r(e, &(rs.rcsr), &offset);
      break;
    }
    case F_FORCE_SINGLE_COIL:
    case F_PRESET_SINGLE_REGISTER: {
      __emb_force_single_coil_r(e, &(rs.fscr), &offset);
      break;
    }
    default:
      return CERR_GENERAL_ERROR;
    }
    e->process_response(e->parse_buffer, e->parse_buffer_offset, &(rs));
  } else {
    if (qs.header.slave_address != e->address) {
      return CERR_OK;
    }
    memcpy(&(qs.header), &qh, sizeof(struct query_header));
    /* else queries are incoming */
    switch (qh.function) {
    case F_READ_COIL_STATUS:
    case F_READ_INPUT_STATUS:
    case F_READ_INPUT_REGISTERS: {
      __emb_read_coil_status_q(e, &(qs.rcsq), &offset);
      break;
    }
    case F_FORCE_SINGLE_COIL:
    case F_PRESET_SINGLE_REGISTER: {
      __emb_force_single_coil_q(e, &(qs.fscq), &offset);
      break;
    }
    default:
      return CERR_GENERAL_ERROR;
    }
    e->process_query(e->parse_buffer, e->parse_buffer_offset, &(qs));
  }

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

int emb_read_coil_status_query(struct read_coil_status_q *rcsq, uint16_t starting_address, uint16_t number_of_points) {
  rcsq->starting_address = starting_address;
  rcsq->number_of_points = number_of_points;
  return CERR_OK;
}

int emb_read_coil_status_response(struct read_coil_status_r *rcsr, uint8_t byte_count, uint8_t *data) {
  rcsr->byte_count = byte_count;
  rcsr->data = data;
  return CERR_OK;
}

int emb_read_holding_registers_response(struct read_holding_registers_r *rhrr, uint8_t byte_count, uint16_t *data) {
  rhrr->byte_count = byte_count;
  rhrr->data = data;
  return CERR_OK;
}

int emb_force_single_coil_query(struct force_single_coil_q *fscq, uint16_t address, uint16_t data) {
  fscq->address = address;
  fscq->data = data;
  return CERR_OK;
}

uint8_t __lrc_add_byte(uint8_t lrc, uint8_t data) {
  lrc = lrc + data;
}

uint8_t __lrc_get(uint8_t lrc) {
  return ((lrc ^ 0xFF) + 1);
}

int emb_query_serialize(struct emb* e, uint8_t function, struct query *q, uint8_t *data_buffer, unsigned int buffer_size) {
  uint8_t *ptr = NULL;
  unsigned int struct_size = 0;
  switch (function) {
  case F_READ_COIL_STATUS:
    ptr = (uint8_t *)&(q->rcsq);
    struct_size = sizeof(q->rcsq);
    break;
  case F_READ_INPUT_STATUS:
    ptr = (uint8_t *)&(q->risq);
    struct_size = sizeof(q->risq);
    break;
  case F_READ_HOLDING_REGISTERS:
    ptr = (uint8_t *)&(q->rhrq);
    struct_size = sizeof(q->rhrq);
    break;
  case F_READ_INPUT_REGISTERS:
    ptr = (uint8_t *)&(q->rirq);
    struct_size = sizeof(q->rirq);
    break;
  case F_FORCE_SINGLE_COIL:
    ptr = (uint8_t *)&(q->fscq);
    struct_size = sizeof(q->fscq);
    break;
  case F_PRESET_SINGLE_REGISTER:
    ptr = (uint8_t *)&(q->psrq);
    struct_size = sizeof(q->psrq);
    break;
  default:
    return CERR_ENOTFOUND;
  }
  if (buffer_size < (2*(sizeof(q->header)+struct_size)+CONTAINER_OVERHEAD)) {
    return CERR_ENOMEM;
  }
  uint8_t *p = (uint8_t *)&(q->header);
  uint8_t hi;
  uint8_t lo;
  uint8_t lrc = 0;
  data_buffer[0] = ':';
  int i = 1;
  for (; i < 2*sizeof(struct query_header); i+=2) {
    __byte_to_hex(*p, &hi, &lo);
    data_buffer[i] = hi;
    data_buffer[i+1] = lo;
    lrc = __lrc_add_byte(lrc, *p);
    p++;
  }
  p = ptr;
  for (; i < 2*struct_size; i+=2) {
    __byte_to_hex(*p, &hi, &lo);
    data_buffer[i] = hi;
    data_buffer[i+1] = lo;
    lrc = __lrc_add_byte(lrc, *p);
    p++;
  }
  lrc = __lrc_get(lrc);

  return 2*(sizeof(struct query_header) + struct_size);
}

int emb_response_serialize(struct emb* e, uint8_t function, struct response *r, uint8_t *data_buffer, unsigned int buffer_size) {
  return 0;
}
