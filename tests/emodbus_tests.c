#include "CUnit/Basic.h"
#include <emodbus_parser.h>
#include <stdio.h>

int init_emb_test_suite(void)
{
  return 0;
}

int clean_emb_test_suite(void)
{
  return 0;  
}

int n_queries = 0;
int n_responses = 0;

void process_response(const void * data, unsigned int size, struct response *rs) {
  n_responses += 1;
  int i = 0;
  char * data_ptr = (char*)data;
  printf("function: %i\r\n", rs->header.function);
  /* switch (rs->header.function) { */
    
  /* } */
  printf("\r\nprocess response: ");
  for (;i<size;i++) {
    printf("%c", data_ptr[i]);
  }
}

void process_query(const void * data, unsigned int size, struct query *qs) {
  int i = 0;
  char * data_ptr = (char*)data;
  printf("function: %i\r\n", qs->header.function);
  printf("\r\nprocess query: ");
  for (;i<size;i++) {
    printf("%c", data_ptr[i]);
  }
}


void test_emb_responses(void)
{
  char *buf = ":0603006Baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0003AB\r\ngjksdfg:0603006b0003AB\r\nsdghsdfdfghdfhsfh:110105cd6bb20e1b00\r\nsdfglbhawset";
  uint8_t storage[32];
  struct emb e;
  emb_init(&e, storage, 32, process_response, process_query, 1);
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
  
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  printf ("running tests\n");
  /* CU_automated_run_tests(); */
  CU_cleanup_registry();
  return CU_get_error();
}
