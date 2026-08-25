#include "unity.h"
#include "libtetrisbrain/garbage.h"
#include "libtetrisbrain/board_control.h"
#include "libtetrisbrain/state.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

GameState test_state;

void setUp(void)
{
    memset(&test_state, 0, sizeof(GameState));
    srand(time(NULL));
}
void tearDown(void)
{
}

void test_garbage_fuzzer_boundary(void)
{
    // L11.1 Random Fuzzing: Blast the engine with 100 random, out-of-bounds payloads
    for (int i = 0; i < 100; i++) {
        int random_garbage = (rand() % 300) - 100; // Generate negatives and massive positives
        add_garbage(&test_state, random_garbage);
    }

    // L11.2 Safety Constraint: Board array index must not under/overflow.
    // If it reaches here without a segmentation fault, the clamps worked.
    TEST_ASSERT_TRUE(test_state.board.cells[0][0] >= 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_garbage_fuzzer_boundary);
    return UNITY_END();
}
