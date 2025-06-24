#include <gtest/gtest.h>
#include "../../sdr/pseudorandom_phase.hpp"

// Test that the random generator returns a float
TEST(PseudorandomPhase, ReturnValidFloat) {
    EXPECT_NO_THROW(get_next_phase(true));
    EXPECT_NO_THROW(get_next_phase(false));
}