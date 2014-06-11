#include "CUnit/Basic.h"
#include <emodbus_parser.h>
#include <stdio.h>

#define ADDRESS 1

int init_emb_test_suite(void) {
  return 0;
}

int clean_emb_test_suite(void) {
  return 0;  
}

int n_queries = 0;
int n_responses = 0;

void process_response(const void * data, unsigned int size, struct response *rs) {
  n_responses += 1;
  int i = 0;
  char * data_ptr = (char*)data;
  printf("function: %i\r\n", rs->header.function);
  printf("process response: ");
  for (;i<size;i++) {
    printf("%c", data_ptr[i]);
  }
}

void process_query(const void * data, unsigned int size, struct query *qs) {
  n_queries += 1;
  int i = 0;
  char * data_ptr = (char*)data;
  printf("function: %i\r\n", qs->header.function);
  printf("\r\nprocess query: ");
  for (;i<size;i++) {
    printf("%c", data_ptr[i]);
  }
}

void test_ring(void) {
  char buf[1024];
  memset(buf, 0, 1024);
  uint8_t storage_m[32];
  uint8_t storage_s[32];
  n_queries = 0;
  n_responses = 0;
  struct emb e_m;
  struct emb e_s;

  // address doesnt matter for master, lets set it to 0
  emb_init(&e_m, storage_m, 32, process_response, process_query, EMB_MASTER, 0);
  emb_init(&e_s, storage_s, 32, process_response, process_query, EMB_SLAVE, ADDRESS);
  //create some master query
  struct query m_q;
  emb_fill_header(&m_q.header, ADDRESS, F_READ_COIL_STATUS);
  emb_read_coil_status_query(&m_q.rcsq, 0, 1);
  int len = emb_query_serialize(&e_m, F_READ_COIL_STATUS, &m_q, buf, 1024);

  //serialized message is in the buf, lets send it to slave
  printf("pushing %i [%s] bytes to slave\r\n", len, buf);
  emb_push_data(&e_s, buf, len);
  emb_process_data(&e_s);
  CU_ASSERT(n_queries == 1);
  
  //serialize response from slave to master
  memset(buf, 0, 1024);
  struct response s_r;
  emb_fill_header(&s_r.header, ADDRESS, F_READ_COIL_STATUS);
  uint8_t coil = 0;
  emb_read_coil_status_response(&s_r.rcsr, 1, &coil);
  len = emb_response_serialize(&e_s, F_READ_COIL_STATUS, &s_r, buf, 1024);

  //push to master
  printf("pushing %i [%s] bytes to slave\r\n", len, buf);
  emb_push_data(&e_m, buf, len);
  emb_process_data(&e_m);
  CU_ASSERT(n_responses == 1);
}

void test_emb_responses(void) {
  char *buf = ":0603006Baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0003AB\r\ngjksdfg:0603006b0003AB\r\nsdghsdfdfghdfhsfh:210105cd6bb20e1b00\r\n:010105cd6bb20e1b00\r\nsdfglbhawset";
  uint8_t storage[32];
  struct emb e;
  emb_init(&e, storage, 32, process_response, process_query, EMB_MASTER, ADDRESS);
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

  CU_ASSERT(n_responses==2);
}

void test_emb_queries(void)
{
  char *buf = ":0603006Baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0003AB\r\ngjksdfg:0603006b0003AB\r\nsdghsdfdfghdfhsfh:210105cd6bb20e1b00\r\n:010105cd6bb20e1b00\r\nsdfglbhawset";
  uint8_t storage[32];
  struct emb e;
  emb_init(&e, storage, 32, process_response, process_query, EMB_SLAVE, ADDRESS);
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

  CU_ASSERT(n_queries==1);
}

int main()
{
  CU_pSuite pSuite = NULL;
  
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();
  
  pSuite = CU_add_suite("emodbus suite", init_emb_test_suite, clean_emb_test_suite);
  if (NULL == pSuite)
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_ADD_TEST(pSuite, test_emb_responses))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_ADD_TEST(pSuite, test_emb_queries))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_ADD_TEST(pSuite, test_ring))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  printf ("running tests\n");
  /* CU_automated_run_tests(); */
  CU_cleanup_registry();
  return CU_get_error();
}
