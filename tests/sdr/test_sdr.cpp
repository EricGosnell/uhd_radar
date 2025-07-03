#include "../../sdr/sdr.hpp"
#include "../../sdr/rf_settings.hpp"
#include <gtest/gtest.h>

using namespace std;

class SdrHwTest{
    public:
    void revealCheck10MhzLock(Sdr& sdr){sdr.check10MhzLock();}
    void revealGpsLock(Sdr& sdr){sdr.gpsLock();}
    void revealCheckAndSetTime(Sdr& sdr){sdr.checkAndSetTime();}
    void revealDetectChannels(Sdr& sdr){sdr.detectChannels();}
    void revealSetRFParams(Sdr& sdr){sdr.setRFParams();}
    void revealRefLoLockDetect(Sdr& sdr){sdr.refLoLockDetect();}
    void revealSetupGpio(Sdr& sdr){sdr.setupGpio();}
    void revealSetupTx(Sdr& sdr){sdr.setupTx();}
    void revealSetupRx(Sdr& sdr){sdr.setupRx();}
};
/**
 * @brief tests that the construction of sdr class reads from yaml correctly
 * 
 * Has unit tests for all variables assigned in the loadConfigFromYaml 
 * fucntion in the sdr class to make sure the function pulls from Yaml
 * correctly
 */
TEST(loadConfigFromYaml, LoadsDefault){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    //DEVICE
    EXPECT_EQ(sdr.getDeviceArgs(), "num_recv_frames=700,num_send_frames=700,recv_frame_size=11000,send_frame_size=11000");
    EXPECT_EQ(sdr.getSubdev(), "A:A");
    EXPECT_EQ(sdr.getClkRef(), "internal");
    EXPECT_EQ(sdr.getClkRate(), 56e6);
    EXPECT_EQ(sdr.getTxChannels(), "0");
    EXPECT_EQ(sdr.getRxChannels(), "0");
    EXPECT_EQ(sdr.getCpuFormat(), "fc32");
    EXPECT_EQ(sdr.getOtwFormat(), "sc12");

    //GPIO
    EXPECT_EQ(sdr.getPwrAmpPin(), -3);
    EXPECT_EQ(sdr.getGpioBank(), "FP0");
    EXPECT_EQ(sdr.getRefOutInt(), -1);

    //RF
    EXPECT_EQ(sdr.getRxRate(), 56e6);
    EXPECT_EQ(sdr.getTxRate(), 56e6);
    EXPECT_EQ(sdr.getFreq(), 450e6);
    EXPECT_EQ(sdr.getRxGain(), 10);
    EXPECT_EQ(sdr.getTxGain(), 10);
    EXPECT_EQ(sdr.getBw(), 56e6);
    EXPECT_EQ(sdr.getTxAnt(), "TX/RX");
    EXPECT_EQ(sdr.getRxAnt(), "RX2");
    EXPECT_EQ(sdr.getTransmit(), true);
}

/**
 * @brief tests GPIO mask getter functions
 *
 * these 4 getter methods are not tested with the other getter
 * methods in Loads Default because they are not directly read from YAML
 */
TEST(GetterMethods, UntestedInLoadsDefault){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);

    EXPECT_EQ(sdr.getAmpGpioMask(), 536870912); // 536870912 in 32-bit binary is 00100000000000000000000000000000
    EXPECT_EQ(sdr.getAtrMasks(), 536870912);
    EXPECT_EQ(sdr.getAtrControl(), 536870912);
    EXPECT_EQ(sdr.getGpioDdr(), 536870912);
}


//hardware testing for usrp

TEST(gpsLock, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    test.revealGpsLock(sdr);
    EXPECT_TRUE(sdr.getUsrp()->get_mboard_sensor("gps_locked", 0).to_bool());
}