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
    string yaml_filename = "../../../config/default.yaml";
    Sdr sdr(yaml_filename);
    //DEVICE
    EXPECT_EQ(sdr.device_args, "num_recv_frames=700,num_send_frames=700,recv_frame_size=11000,send_frame_size=11000");
    EXPECT_EQ(sdr.subdev, "A:A");
    EXPECT_EQ(sdr.clk_ref, "internal");
    EXPECT_EQ(sdr.clk_rate, 56e6);
    EXPECT_EQ(sdr.tx_channels, "0");
    EXPECT_EQ(sdr.rx_channels, "0");
    EXPECT_EQ(sdr.cpu_format, "fc32");
    EXPECT_EQ(sdr.otw_format, "sc12");

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
