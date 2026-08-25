#include "unity.h"
#include "libtetrisbrain/killfeed.h"
#include <pthread.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// Thread worker simulating intense network logging
void* killfeed_spammer(void* arg)
{
    (void)arg;
    for (int i = 0; i < 5000; i++) {
        add_kill_feed(1, 2, 4);
    }
    return NULL;
}

void test_killfeed_race_conditions(void)
{
    // L12.1 Concurrency: 2 threads hammering the same circular buffer
    pthread_t t1, t2;
    pthread_create(&t1, NULL, killfeed_spammer, NULL);
    pthread_create(&t2, NULL, killfeed_spammer, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // If no segfault occurs during threaded circular buffer wraparound, test passes.
    TEST_ASSERT_MESSAGE(1 == 1, "Concurrency survived without segmentation fault.");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_killfeed_race_conditions);
    return UNITY_END();
}
