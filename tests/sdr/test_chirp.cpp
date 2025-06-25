#include "../../sdr/chirp.hpp"
#include <gtest/gtest.h>

using namespace std;

/**
 * @brief tests that the construction of chirp class reads from yaml correctly
 * 
 * Has unit tests for all variables assigned in the assignVarFromUYam; 
 * fucntion in the chirp class to make sure the function pulls from Yaml
 * correctly
 */
TEST(assignVarFromYaml, loadsDefault){

    const string kConfigFile = string(CONFIG_DIR) + "/default.yaml";
    Chirp chirp(kConfigFile);

    EXPECT_EQ(chirp.time_offset, 1);
    EXPECT_EQ(chirp.tx_duration, 20e-6);
    EXPECT_EQ(chirp.rx_duration, 20e-6);
    EXPECT_EQ(chirp.tr_on_lead, 0e-6);
    EXPECT_EQ(chirp.tr_off_trail, 0e-6);
    EXPECT_EQ(chirp.pulse_rep_int, 200e-6);
    EXPECT_EQ(chirp.tx_lead, 0e-6);
    EXPECT_EQ(chirp.num_pulses, 10000);
    EXPECT_EQ(chirp.num_presums, 1);
    EXPECT_EQ(chirp.phase_dither, true);
}