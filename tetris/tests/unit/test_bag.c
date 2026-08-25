#include "unity.h"
#include "libtetrisbrain/bag.h"
#include "libtetrisbrain/state.h"
#include <string.h>

GameState test_state;

void setUp(void)
{
    memset(&test_state, 0, sizeof(GameState));
}
void tearDown(void)
{
}

void test_bag_equivalence_partitioning(void)
{
    int new_bag[7] = {1, 2, 3, 4, 5, 6, 7};
    shuffle_array(new_bag, 7);

    // L9.1 Equivalence: The sum of a valid 7-bag MUST be exactly 28.
    int sum = 0;
    for (int i = 0; i < 7; i++)
        sum += new_bag[i];
    TEST_ASSERT_EQUAL_INT(28, sum);
}

void test_bag_boundary_transition(void)
{
    test_state.bag_index = 6; // Boundary: 7th piece
    spawn_new_piece(&test_state);
    // L9.1 Boundary Value Analysis: Index should wrap back to 0
    TEST_ASSERT_EQUAL_INT(0, test_state.bag_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bag_equivalence_partitioning);
    RUN_TEST(test_bag_boundary_transition);
    return UNITY_END();
}
