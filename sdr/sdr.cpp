#include "sdr.hpp"

/**
* @brief Constructs a new Sdr object
*
* Constructs a new Sdr object and calls the relevant functions
* that initialize each member variable from the specified YAML file.
*
* @param kYamlFile Path to the YAML configuration file (config/)
*/
Sdr::Sdr(const string& kYamlFile) {
  load_config_from_yaml(kYamlFile);
}

/**
* @brief Load configuration from YAML file into the SDR class
*
* Reads in SDR and hardware-related configuration settings from the
* provided YAML file, and stores them in the corresponding member
* variables of the SDR class.
*
* @param kYamlFile Path to the YAML configuration file (config/)
*/
void Sdr::load_config_from_yaml(const string& kYamlFile) {
  YAML::Node config = YAML::LoadFile(kYamlFile);

  // Device
  YAML::Node dev_params = config["DEVICE"];
  subdev = dev_params["subdev"].as<string>();
  clk_ref = dev_params["clk_ref"].as<string>();
  device_args = dev_params["device_args"].as<string>();
  clk_rate = dev_params["clk_rate"].as<double>();
  tx_channels = dev_params["tx_channels"].as<string>();
  rx_channels = dev_params["rx_channels"].as<string>();
  cpu_format = dev_params["cpu_format"].as<string>("fc32");
  otw_format = dev_params["otw_format"].as<string>();

  // GPIO
  YAML::Node gpio_params = config["GPIO"];
  gpio_bank = gpio_params["gpio_bank"].as<string>();
  pwr_amp_pin = gpio_params["pwr_amp_pin"].as<int>();
  pwr_amp_pin -= 2; // map the specified DB15 pin to the GPIO pin numbering
  if (pwr_amp_pin != -1) {
    AMP_GPIO_MASK = (1 << pwr_amp_pin);
    ATR_MASKS = (AMP_GPIO_MASK);
    ATR_CONTROL = (AMP_GPIO_MASK);
    GPIO_DDR = (AMP_GPIO_MASK);
  }

  ref_out_int = gpio_params["ref_out"].as<int>();

  // RF
  YAML::Node rf0 = config["RF0"];
  YAML::Node rf1 = config["RF1"];
  rx_rate = rf1["rx_rate"].as<double>();
  tx_rate = rf1["tx_rate"].as<double>();
  freq = rf1["freq"].as<double>();
  rx_gain = rf1["rx_gain"].as<double>();
  tx_gain = rf1["tx_gain"].as<double>();
  bw = rf1["bw"].as<double>();
  tx_ant = rf1["tx_ant"].as<string>();
  rx_ant = rf1["rx_ant"].as<string>();

  transmit = rf0["transmit"].as<bool>(true); // True if transmission enabled
}
