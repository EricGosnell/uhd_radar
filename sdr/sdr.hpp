#pragma once
#include <string>
#include <cstdint>
#include "yaml-cpp/yaml.h"

using namespace std;

class Sdr {
  public:
      // DEVICE
      string device_args;
      string subdev;
      string clk_ref;
      double clk_rate;
      string tx_channels;
      string rx_channels;
      string cpu_format;
      string otw_format;

      // GPIO
      int pwr_amp_pin;
      string gpio_bank;
      uint32_t AMP_GPIO_MASK;
      uint32_t ATR_MASKS;
      uint32_t ATR_CONTROL;
      uint32_t GPIO_DDR;
      int ref_out_int;

      // RF
      double rx_rate;
      double tx_rate;
      double freq;
      double rx_gain;
      double tx_gain;
      double bw;
      string tx_ant;
      string rx_ant;
      bool transmit;

  private:
    void load_config_from_yaml(const string& kYamlFile);
};
