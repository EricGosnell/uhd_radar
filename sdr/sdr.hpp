#ifndef SDR_HPP
#define SDR_HPP

#include "yaml-cpp/yaml.h"
#include "rf_settings.hpp"
#include "common.hpp"

class Sdr {
  public:
    Sdr(const string& kYamlFile);

    // RF
    YAML::Node rf0;
    YAML::Node rf1;
    double rx_rate;
    double tx_rate;
    double freq;
    double rx_gain;
    double tx_gain;
    double bw;
    string tx_ant;
    string rx_ant;
    bool transmit;

    //USRP
    usrp::multi_usrp::sptr usrp;
    tx_streamer::sptr tx_stream;
    rx_streamer::sptr rx_stream;

    vector<string> tx_channel_strings;
    vector<size_t> tx_channel_nums;

    vector<string> rx_channel_strings;
    vector<size_t> rx_channel_nums;
    

    //functions
    void createUsrp();
    void setupUsrp();

    // DEVICE
    string getDeviceArgs() const;
    string getSubdev() const;
    string getClkRef() const;
    double getClkRate() const;
    string getTxChannels() const;
    string getRxChannels() const;
    string getCpuFormat() const;
    string getOtwFormat() const;

    // GPIO
    int getPwrAmpPin() const;
    string getGpioBank() const;
    uint32_t getAmpGpioMask() const;
    uint32_t getAtrMasks() const;
    uint32_t getAtrControl() const;
    uint32_t getGpioDdr() const;
    int getRefOutInt() const;

  private:
    void loadConfigFromYaml(const string& kYamlFile);
    void check10MhzLock();
    void gpsLockAndTime();
    void checkTime(time_spec_t& gps_time);
    void detectChannels();
    void setRFParams();
    void refLoLockDetect();
    void setupGpio();
    void setupTx();
    void setupRx();

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
    int pwr_amp_pin;        // Which GPIO pin to use for external power amplifier control (set to -1 if not using)
    string gpio_bank;       // Which GPIO bank to use (FP0 is front panel and default)
    uint32_t AMP_GPIO_MASK;
    uint32_t ATR_MASKS;
    uint32_t ATR_CONTROL;
    uint32_t GPIO_DDR;
    int ref_out_int;        // Turns the 10 MHz reference out signal on (1) or off (0)
                            // set to (-1) if SDR does not support


};

#endif
