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
