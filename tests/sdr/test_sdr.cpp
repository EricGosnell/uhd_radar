#include "../../sdr/sdr.hpp"
#include "../../sdr/rf_settings.hpp"
#include <gtest/gtest.h>

using namespace std;

TEST(loadConfigFromYaml, LoadsDefault){
    string yaml_filename = "../../../config/default.yaml";
    Sdr sdr(yaml_filename);
    EXPECT_EQ(sdr.loadConfigFromYaml(yaml_filename), "num_recv_frames=700,num_send_frames=700,recv_frame_size=11000,send_frame_size=11000");
}
