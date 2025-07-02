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

    EXPECT_EQ(chirp.getTimeOffset(), 1);
    EXPECT_EQ(chirp.getTxDuration(), 20e-6);
    EXPECT_EQ(chirp.getRxDuration(), 20e-6);
    EXPECT_EQ(chirp.getTrOnLead(), 0e-6);
    EXPECT_EQ(chirp.getTrOffTrail(), 0e-6);
    EXPECT_EQ(chirp.getPulseRepInt(), 200e-6);
    EXPECT_EQ(chirp.getTxLead(), 0e-6);
    EXPECT_EQ(chirp.getNumPulses(), 10000);
    EXPECT_EQ(chirp.getNumPresums(), 1);
    EXPECT_EQ(chirp.getPhaseDither(), true);
}
