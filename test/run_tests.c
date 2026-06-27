#include "unity.h"

void run_cpu_tests(void);
void run_ppu_tests(void);
void setUp(void);
void tearDown(void);

void setUp(void)
{
   ;
}

void tearDown(void)
{
   ;
}

int main(void)
{
   UNITY_BEGIN();

   TEST_MESSAGE("Running unit tests ...");

   RUN_TEST(run_cpu_tests);
   RUN_TEST(run_ppu_tests);

   TEST_MESSAGE("Done.");

   return UNITY_END();
}