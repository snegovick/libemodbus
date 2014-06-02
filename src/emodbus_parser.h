#ifndef __PARSER_H__
#define __PARSER_H__

#include <circularbuf.h>
#include <string.h>

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
    struct read_input_status_q risq;
    struct read_input_registers_q rirq;
    struct preset_single_register_q psrq;
  };
};

struct response {
  struct query_header header;
  union {
    struct read_coil_status_r rcsr;
    struct read_input_status_r risr;
    struct read_holding_registers_r rhrr;
    struct force_single_coil_r fscr;
    struct preset_single_register_r psrr;
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

int emb_init(struct emb *e, uint8_t *storage, unsigned int storage_size,
             void (*process_response)(const void *data, unsigned int size, struct response *rs), 
             void (*process_query)(const void *data, unsigned int size, struct query *qs), int master);
void emb_recover(struct emb *e);
int emb_push_data(struct emb *e, const uint8_t *new_data, unsigned int data_size);
int emb_process_data(struct emb *e);

#endif/*__PARSER_H__*/
