#include "../../sdr/sdr.hpp"
#include "../../sdr/rf_settings.hpp"
#include <gtest/gtest.h>

using namespace std;

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
    EXPECT_EQ(sdr.pwr_amp_pin, -3);
    EXPECT_EQ(sdr.gpio_bank, "FP0");
    EXPECT_EQ(sdr.ref_out_int, -1);

    //RF
    EXPECT_EQ(sdr.rx_rate, 56e6);
    EXPECT_EQ(sdr.tx_rate, 56e6);
    EXPECT_EQ(sdr.freq, 450e6);
    EXPECT_EQ(sdr.rx_gain, 10);
    EXPECT_EQ(sdr.tx_gain, 10);
    EXPECT_EQ(sdr.bw, 56e6);
    EXPECT_EQ(sdr.tx_ant, "TX/RX");
    EXPECT_EQ(sdr.rx_ant, "RX2");
    EXPECT_EQ(sdr.transmit, true);
}
