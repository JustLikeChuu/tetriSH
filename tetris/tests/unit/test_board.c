#include "unity.h"
#include "libtetrisbrain/board_control.h"
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

void test_srs_wall_kick_branch(void)
{
    // Setup T-piece jammed DEEP against the left wall
    test_state.current.type = 3;
    test_state.current.x = -2; // Push it deeper into the wall!
    test_state.current.y = 5;
    test_state.current.rot = ROT_1;

    // L10.2 Path Coverage: The 0th kick {0,0} will now definitively fail.
    // Logic MUST fall through to the {1, 0} kick and succeed.
    bool success = test_rotate(&test_state, ROT_2);

    TEST_ASSERT_TRUE(success);
    // If it started at -2 and kicked right by +1, x should now be -1
    TEST_ASSERT_EQUAL_INT(-1, test_state.current.x);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_srs_wall_kick_branch);
    return UNITY_END();
}
