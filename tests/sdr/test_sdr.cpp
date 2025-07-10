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
//add comments the can be read by doxygen like the comments above the other functions
//make unit tests for all funtionality of the functions, edge cases and stuff. Also, mess up variables and see if the function throws errors or still runs.
//test with hardware before merging with main
TEST(check10MhzLock, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    sdr.createUsrp();

    if(sdr.getClkRef() == "gpsdo"){
    test.revealCheck10MhzLock(sdr);
    EXPECT_TRUE(sdr.getUsrp()->get_mboard_sensor("gps_locked", 0).to_bool()); //should pass test if locked to GPSDO 10 Mhz reference
    }
}

TEST(gpsLock, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    sdr.createUsrp();

    if(sdr.getClkRef() == "gpsdo"){
    test.revealCheck10MhzLock(sdr);
    test.revealGpsLock(sdr);
    EXPECT_TRUE(sdr.getUsrp()->get_mboard_sensor("gps_locked", 0).to_bool());
}}

TEST(checkAndSetTime, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    sdr.createUsrp();
    if(sdr.getClkRef() == "gpsdo"){
        test.revealCheck10MhzLock(sdr);
        test.revealGpsLock(sdr);
        test.revealCheckAndSetTime(sdr);
        auto gps_time =sdr.getUsrp()->get_mboard_sensor("gps_time", 0).to_int();
        time_spec_t expected_time(gps_time + 1.0);
        auto actual_time = sdr.getUsrp()->get_time_last_pps(0);
        EXPECT_EQ(expected_time, actual_time); //should pass if USRP time is synchronized to GPS time
    //does this need to exist
}}

TEST(detectChannels, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    sdr.createUsrp();

    sdr.getUsrp()->set_time_next_pps(time_spec_t(0.0));
    this_thread::sleep_for((chrono::milliseconds(1000)));
    if (sdr.getTransmit()) {
        sdr.getUsrp()->set_tx_subdev_spec(sdr.getSubdev());
     }
    sdr.getUsrp()->set_rx_subdev_spec(sdr.getSubdev());
    sdr.getUsrp()->set_master_clock_rate(sdr.getClkRate());

    test.revealDetectChannels(sdr);
    size_t i = sdr.getTxChannelStrings().size() - 1;
    auto tx_chan = stoi(sdr.getTxChannelStrings()[i]);
    EXPECT_GT(sdr.getUsrp()->get_tx_num_channels(), tx_chan);
    EXPECT_NO_THROW(test.revealDetectChannels(sdr)); //makes sure no errors are thrown when working how the function should

    size_t j = sdr.getRxChannelStrings().size()-1;
    auto rx_chan = stoi(sdr.getRxChannelStrings()[j]);
    EXPECT_GT(sdr.getUsrp()->get_rx_num_channels(), rx_chan);
    EXPECT_NO_THROW(test.revealDetectChannels(sdr)); //makes sure no errors are thrown when working how the function should;
    }

TEST(setRFParams, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    sdr.createUsrp();

    sdr.getUsrp()->set_time_next_pps(time_spec_t(0.0));
    this_thread::sleep_for((chrono::milliseconds(1000)));
    if (sdr.getTransmit()) {
        sdr.getUsrp()->set_tx_subdev_spec(sdr.getSubdev());
     }
    sdr.getUsrp()->set_rx_subdev_spec(sdr.getSubdev());
    sdr.getUsrp()->set_master_clock_rate(sdr.getClkRate());

    test.revealDetectChannels(sdr);
    test.revealSetRFParams(sdr);
    EXPECT_NO_THROW(test.revealSetRFParams(sdr));
}

TEST(refLoLockDetect, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    sdr.createUsrp();

    sdr.getUsrp()->set_time_next_pps(time_spec_t(0.0));
    this_thread::sleep_for((chrono::milliseconds(1000)));
    if (sdr.getTransmit()) {
        sdr.getUsrp()->set_tx_subdev_spec(sdr.getSubdev());
     }
    sdr.getUsrp()->set_rx_subdev_spec(sdr.getSubdev());
    sdr.getUsrp()->set_master_clock_rate(sdr.getClkRate());

    test.revealDetectChannels(sdr);
    test.revealSetRFParams(sdr);
    test.revealRefLoLockDetect(sdr);

    EXPECT_NO_THROW(test.revealRefLoLockDetect(sdr));
    }

TEST(setupGpio, checkParams){
    const string kYamlFile = string(CONFIG_DIR) + "/default.yaml";
    Sdr sdr(kYamlFile);
    SdrHwTest test;

    sdr.createUsrp();

    sdr.getUsrp()->set_time_next_pps(time_spec_t(0.0));
    this_thread::sleep_for((chrono::milliseconds(1000)));
    if (sdr.getTransmit()) {
        sdr.getUsrp()->set_tx_subdev_spec(sdr.getSubdev());
     }
    sdr.getUsrp()->set_rx_subdev_spec(sdr.getSubdev());
    sdr.getUsrp()->set_master_clock_rate(sdr.getClkRate());

    test.revealDetectChannels(sdr);
    test.revealSetRFParams(sdr);
    test.revealRefLoLockDetect(sdr);
    test.revealSetupGpio(sdr);
    EXPECT_NE(sdr.getPwrAmpPin(), -1);
}
