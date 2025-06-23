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
  loadConfigFromYaml(kYamlFile);
  createUsrp();
  setupUsrp();
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
void Sdr::loadConfigFromYaml(const string& kYamlFile) {
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
  rf0 = config["RF0"];
  rf1 = config["RF1"];
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


void Sdr::createUsrp(){
  cout << endl;
  cout << boost::format("Creating the usrp device with: %s...")
    % device_args << endl; 
  usrp = uhd::usrp::multi_usrp::make(device_args);
  cout << boost::format("TX/RX Device: %s") % usrp->get_pp_string() << endl;

  // Lock mboard clocks
  usrp->set_clock_source(clk_ref);
  usrp->set_time_source(clk_ref);
}

void Sdr::setupUsrp(){
  if (clk_ref == "gpsdo") {
    check10MhzLock();
    gpsLockAndTime();
  }else{
    // set the USRP time, let chill for a little bit to lock
    usrp->set_time_next_pps(time_spec_t(0.0));
    this_thread::sleep_for((chrono::milliseconds(1000)));
  }
  // always select the subdevice first, the channel mapping affects the
  // other settings
  if (transmit) {
  usrp->set_tx_subdev_spec(subdev);
  }
  usrp->set_rx_subdev_spec(subdev);

  // set master clock rate
  usrp->set_master_clock_rate(clk_rate);
  detectChannels();
  setRFParams();
  refLoLockDetect();
  setupGpio();
  setupTx();
  setupRx();
}


void Sdr::check10MhzLock(){
  vector<string> sensor_names = usrp->get_mboard_sensor_names(0);
    if (find(sensor_names.begin(), sensor_names.end(), "ref_locked")
        != sensor_names.end()) {
        cout << "Waiting for reference lock..." << flush;
        bool ref_locked = false;
        for (int i = 0; i < 30 and not ref_locked; i++) {
            ref_locked = usrp->get_mboard_sensor("ref_locked", 0).to_bool();
            if (not ref_locked) {
                cout << "." << flush;
                this_thread::sleep_for(chrono::seconds(1));
            }
        }
        if (ref_locked) {
            cout << "LOCKED" << endl;
        } else {
            cout << "FAILED" << endl;
            cout << "Failed to lock to GPSDO 10 MHz Reference. Exiting."
                      << endl;
            exit(EXIT_FAILURE);
        }
    } else {
        cout << boost::format(
            "ref_locked sensor not present on this board.\n");
    }
}

void Sdr::gpsLockAndTime(){
  //wait for GPS lock
  bool gps_locked = usrp->get_mboard_sensor("gps_locked", 0).to_bool();
  size_t num_gps_locked = 0;
  for (int i = 0; i < 30 and not gps_locked; i++) {
    gps_locked = usrp->get_mboard_sensor("gps_locked", 0).to_bool();
    if (not gps_locked) {
          cout << "." << flush;
          this_thread::sleep_for(chrono::seconds(1));
      }
    }
  if (gps_locked) {
    num_gps_locked++;
    cout << boost::format("GPS Locked\n");
  } else {
      cerr
          << "WARNING:  GPS not locked - time will not be accurate until locked"
          << endl;
    }

  //set GPS time
    time_spec_t gps_time = time_spec_t(
        int64_t(usrp->get_mboard_sensor("gps_time", 0).to_int()));
    usrp->set_time_next_pps(gps_time + 1.0, 0);

    // Wait for it to apply
    // The wait is 2 seconds because N-Series has a known issue where
    // the time at the last PPS does not properly update at the PPS edge
    // when the time is actually set.
    this_thread::sleep_for(chrono::seconds(2));
    checkTime(gps_time);
}

void Sdr::checkTime(time_spec_t& gps_time){
 gps_time = time_spec_t(
        int64_t(usrp->get_mboard_sensor("gps_time", 0).to_int()));
    time_spec_t time_last_pps = usrp->get_time_last_pps(0);
    cout << "USRP time: "
              << (boost::format("%0.9f") % time_last_pps.get_real_secs())
              << endl;
    cout << "GPSDO time: "
              << (boost::format("%0.9f") % gps_time.get_real_secs()) << std::endl;
    if (gps_time.get_real_secs() == time_last_pps.get_real_secs())
        cout << endl
                  << "SUCCESS: USRP time synchronized to GPS time" << endl
                  << endl;
    else
        std::cerr << endl
                  << "ERROR: Failed to synchronize USRP time to GPS time"
                  << endl
                  << endl;
}


void Sdr::detectChannels(){
  boost::split(tx_channel_strings, tx_channels, boost::is_any_of("\"',"));
  for (size_t ch = 0; ch < tx_channel_strings.size(); ch++) {
    size_t chan = stoi(tx_channel_strings[ch]);
    if (chan >= usrp->get_tx_num_channels()) {
      throw std::runtime_error("Invalid TX channel(s) specified.");
    } else
      tx_channel_nums.push_back(stoi(tx_channel_strings[ch]));
  }
  boost::split(rx_channel_strings, rx_channels, boost::is_any_of("\"',"));
  for (size_t ch = 0; ch < rx_channel_strings.size(); ch++) {
    size_t chan = stoi(rx_channel_strings[ch]);
    if (chan >= usrp->get_rx_num_channels()) {
      throw std::runtime_error("Invalid RX channel(s) specified.");
    } else
      rx_channel_nums.push_back(stoi(rx_channel_strings[ch]));
  }
}

void Sdr::setRFParams(){
 // set the RF parameters based on 1 or 2 channel operation
  if (tx_channel_nums.size() == 1) {
    set_rf_params_single(usrp, rf0, rx_channel_nums, tx_channel_nums);
  } else if (tx_channel_nums.size() == 2) {
    if (!transmit) {
      throw std::runtime_error("Non-transmit mode not supported by set_rf_params_multi");
    }
    set_rf_params_multi(usrp, rf0, rf1, rx_channel_nums, tx_channel_nums);
  } else {
    throw std::runtime_error("Number of channels requested not supported");
  }

  // allow for some setup time
  this_thread::sleep_for(chrono::seconds(1));
}

void Sdr::refLoLockDetect(){
 // Check Ref and LO Lock detect
  vector<std::string> tx_sensor_names, rx_sensor_names;
  if (transmit) {
    for (size_t ch = 0; ch < tx_channel_nums.size(); ch++) {
      // Check LO locked
      tx_sensor_names = usrp->get_tx_sensor_names(ch);
      if (find(tx_sensor_names.begin(), tx_sensor_names.end(), "lo_locked") != tx_sensor_names.end())
      {
        sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked", ch);
        cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string()
            << endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
      }
    }
  }

  for (size_t ch = 0; ch < rx_channel_nums.size(); ch++) {
    // Check LO locked
    rx_sensor_names = usrp->get_rx_sensor_names(ch);
    if (find(rx_sensor_names.begin(), rx_sensor_names.end(), "lo_locked") != rx_sensor_names.end())
    {
      sensor_value_t lo_locked = usrp->get_rx_sensor("lo_locked", ch);
      cout << boost::format("Checking RX: %s ...") % lo_locked.to_pp_string()
           << endl;
      UHD_ASSERT_THROW(lo_locked.to_bool());
    }
  }
}

void Sdr::setupGpio(){
  cout << "Available GPIO banks: " << std::endl;
  auto banks = usrp->get_gpio_banks(0);
  for (auto& bank : banks) {
      cout << "* " << bank << std::endl;
  }

  // basic ATR setup
  if (pwr_amp_pin != -1) {
    usrp->set_gpio_attr(gpio_bank, "CTRL", ATR_CONTROL, ATR_MASKS);
    usrp->set_gpio_attr(gpio_bank, "DDR", GPIO_DDR, ATR_MASKS);

    // set amp output pin as desired (on only when TX)
    usrp->set_gpio_attr(gpio_bank, "ATR_0X", 0, AMP_GPIO_MASK);
    usrp->set_gpio_attr(gpio_bank, "ATR_RX", 0, AMP_GPIO_MASK);
    usrp->set_gpio_attr(gpio_bank, "ATR_TX", 0, AMP_GPIO_MASK);
    usrp->set_gpio_attr(gpio_bank, "ATR_XX", AMP_GPIO_MASK, AMP_GPIO_MASK);
  }

  //cout << "sdr.AMP_GPIO_MASK: " << bitset<32>(sdr.AMP_GPIO_MASK) << endl;

  // turns external ref out port on or off
   if (ref_out_int == 1) {
    usrp->set_clock_source_out(true);
  } else if (ref_out_int == 0) {
    usrp->set_clock_source_out(false);
  } // else do nothing (SDR likely doesn't support this parameter)
  
  // update the offset time for start of streaming to be offset from the current usrp time
}

void Sdr::setupTx(){
  // Stream formats
  stream_args_t tx_stream_args(cpu_format, otw_format);
  tx_stream_args.channels = tx_channel_nums;

  // tx streamer
  if (transmit) {
    tx_stream = usrp->get_tx_stream(tx_stream_args);
    cout << "INFO: tx_stream get_max_num_samps: " << tx_stream->get_max_num_samps() << endl;
  }
}

void Sdr::setupRx(){
   stream_args_t rx_stream_args(cpu_format, otw_format);

  // rx streamer
  rx_stream_args.channels = rx_channel_nums;
  rx_stream = usrp->get_rx_stream(rx_stream_args);

  cout << "INFO: rx_stream get_max_num_samps: " << rx_stream->get_max_num_samps() << endl;
}