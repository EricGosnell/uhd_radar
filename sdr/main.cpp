#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/convert.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <filesystem>
#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/write.hpp>

#include "yaml-cpp/yaml.h"

#include "rf_settings.hpp"
#include "pseudorandom_phase.hpp"
#include "utils.hpp"
#include "sdr.hpp"
#include "chirp.hpp"

using namespace std;
using namespace uhd;

/*
 * PROTOTYPES
 */
void transmit_worker(tx_streamer::sptr& tx_stream, rx_streamer::sptr& rx_stream, Chirp& chirp, Sdr& sdr);

/*
 * SIG INT HANDLER
 */
static bool stop_signal_called = false;
void sig_int_handler(int) {
  stop_signal_called = true;
}

/*** CONFIGURATION PARAMETERS ***/

// FILENAMES
string chirp_loc;
string output_dir;
string save_loc;
string gps_save_loc;

// Calculated Parameters
double tr_off_delay; // Time before turning off GPIO
size_t num_tx_samps; // Total samples to transmit per chirp
size_t num_rx_samps; // Total samples to receive per chirp

// Global state
long int pulses_scheduled = 0;
long int pulses_received = 0;
long int error_count = 0;
long int last_pulse_num_written = -1; // Index number (pulses_received - error_count) of last sample written to outfile

// Cout mutex
std::mutex cout_mutex;

/*
 * UHD_SAFE_MAIN
 */
int UHD_SAFE_MAIN(int argc, char *argv[]) {

  /** Load YAML file **/

  string yaml_filename;
  if (argc >= 2) {
    yaml_filename = "../../" + string(argv[1]);
  } else {
    yaml_filename = "../../config/default.yaml";
  }
  cout << "Reading from config file: " << yaml_filename << endl;

  Sdr sdr(yaml_filename);
  Chirp chirp(yaml_filename);
  YAML::Node config = YAML::LoadFile(yaml_filename);
  sdr.createUsrp();
  sdr.setupUsrp();

  //YAML::Node rf0 = config["RF0"];
 // YAML::Node rf1 = config["RF1"];

  YAML::Node files = config["FILES"];
  chirp_loc = files["chirp_loc"].as<string>();
  output_dir = files["output_dir"].as<string>();
  save_loc = files["save_loc"].as<string>();
  gps_save_loc = files["gps_loc"].as<string>();
  chirp.max_chirps_per_file = files["max_chirps_per_file"].as<int>();

  //Merge save_loc and gps_save_loc with output_dir
  save_loc = (std::filesystem::path(output_dir) / save_loc).string();
  gps_save_loc = (std::filesystem::path(output_dir) / gps_save_loc).string();

  // Calculated parameters

  tr_off_delay = chirp.tx_duration + chirp.tr_off_trail; // Time before turning off GPIO
  num_tx_samps = sdr.tx_rate * chirp.tx_duration; // Total samples to transmit per chirp // TODO: Should use ["GENERATE"]["sample_rate"] instead!
  num_rx_samps = sdr.rx_rate * chirp.rx_duration; // Total samples to receive per chirp // TODO: Should use ["GENERATE"]["sample_rate"] instead!


  /** Thread, interrupt setup **/

  set_thread_priority_safe(1.0, true);
  
  signal(SIGINT, &sig_int_handler);

  /*** VERSION INFO ***/

  // Note: This print statement is used by automated post-processing code. Please be careful about changing the format.
  cout << "[VERSION] 0.0.1" << endl; // Version numbers: First number:  Increment for major new versions
                                     //                  Second number: Increment for any changes that you expect to matter to post-processing
                                     //                  Third number:  Increment for any change
  // Human-readable notes -- explain notable behavior for humans
  cout << "Note: Phase inversion is performed in this code." << endl;
  cout << "Note: Pre-summing is supported. If used, each sample written will have num_presums error-free samples averaged in." << endl;
  cout << "Note: Nothing is written to the file for error pulses." << endl;
  cout << "Note: A full num_pulses of error-free chirp data will be collected. ";
  cout << "(Total number of TX chirps will be num_pulses + # errors)" << endl; 
  
  cout << "INFO: Number of TX samples: " << num_tx_samps << endl;  //needs to be after chirp and sdr object are both made
  cout << "INFO: Number of RX samples: " << num_rx_samps << endl << endl;  //needs to be after chirp and sdr object are both made

 
  // update the offset time for start of streaming to be offset from the current usrp time
  chirp.time_offset = chirp.time_offset + time_spec_t(sdr.usrp->get_time_now()).get_real_secs();  //needs to be after chirp and sdr object are both made

  /*** SPAWN THE TX THREAD ***/
  boost::thread_group transmit_thread;
  transmit_thread.create_thread(boost::bind(&transmit_worker, sdr.tx_stream, sdr.rx_stream, boost::ref(chirp), boost::ref(sdr)));
  
  if (!sdr.transmit) {
    cout << "WARNING: Transmit disabled by configuration file!" << endl;
  }

  //////////////////////////////////////////////////////////////////////////////////////////

  /*** FILE WRITE SETUP ***/
  boost::asio::io_service ioservice;

  if (save_loc[0] != '/') {
    save_loc = "../../" + save_loc;
  }
  if (gps_save_loc[0] != '/') {
    gps_save_loc = "../../" + gps_save_loc;
  }

  int gps_file = open(gps_save_loc.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
  if (gps_file == -1) {
      throw std::runtime_error("Failed to open GPS file: " + gps_save_loc);
  }

  boost::asio::posix::stream_descriptor gps_stream{ioservice, gps_file};
  auto gps_asio_handler = [](const boost::system::error_code& ec, std::size_t) {
    if (ec.value() != 0) {
      cout << "GPS write error: " << ec.message() << endl;
    }
  };

  ioservice.run();

  

  // open file for writing rx samples
  ofstream outfile;
  int save_file_index = 0;
  string current_filename = save_loc;
  if (chirp.max_chirps_per_file > 0) {
    // Breaking into multiple files is enabled
    current_filename = current_filename + "." + to_string(save_file_index);
  }

  // Note: This print statement is used by automated post-processing code. Please be careful about changing the format.
  cout << "[OPEN FILE] " << current_filename << endl;
  outfile.open(current_filename, ofstream::binary);

  /*** RX LOOP AND SUM ***/
  if (chirp.num_pulses < 0) {
    cout << "num_pulses is < 0. Will continue to send chirps until stopped with Ctrl-C." << endl;
  }

  string gps_data;

  if (sdr.cpu_format != "fc32") {
    cout << "Only cpu_format 'fc32' is supported for now." << endl;
    // This is because we actually need buff and sample_sum to have the correct
    // data type to facilitate phase modulation and summing. In the future, this could be
    // fixed up so that it can work with any supported cpu_format, but it
    // seems unnecessary right now.
    exit(1);
  }

  // receive buffer
  size_t bytes_per_sample = convert::get_bytes_per_item(sdr.cpu_format);
  vector<complex<float>> sample_sum(num_rx_samps, 0); // Sum error-free RX pulses into this vector

  vector<complex<float>> buff(num_rx_samps); // Buffer sized for one pulse at a time
  vector<void *> buffs;
  for (size_t ch = 0; ch < sdr.rx_stream->get_num_channels(); ch++) {
    buffs.push_back(&buff.front()); // TODO: I don't think this actually works for num_channels > 1
  }
  size_t n_samps_in_rx_buff;
  rx_metadata_t rx_md; // Captures metadata from rx_stream->recv() -- specifically primarily timeouts and other errors

  float inversion_phase; // Store phase to use for phase inversion of this chirp

  // Note: This print statement is used by automated post-processing code. Please be careful about changing the format.
  cout << "[START] Beginning main loop" << endl;

  while ((chirp.num_pulses < 0) || (last_pulse_num_written < chirp.num_pulses)) {

    n_samps_in_rx_buff = sdr.rx_stream->recv(buffs, num_rx_samps, rx_md, 60.0, false); // TODO: Think about timeout

    if (chirp.phase_dither) {
      inversion_phase = -1.0 * get_next_phase(false); // Get next phase from the generator each time to keep in sequence with TX
    }

    if (rx_md.error_code != rx_metadata_t::ERROR_CODE_NONE){
      // Note: This print statement is used by automated post-processing code. Please be careful about changing the format.
      cout_mutex.lock();
      cout << "[ERROR] (Chirp " << pulses_received << ") Receiver error: " << rx_md.strerror() << "\n";
      cout_mutex.unlock();
      
      pulses_received++;
      error_count++;
    } else if (n_samps_in_rx_buff != num_rx_samps) {
      // Unexpected number of samples received in buffer!
      // Note: This print statement is used by automated post-processing code. Please be careful about changing the format.
      cout_mutex.lock();
      cout << "[ERROR] (Chirp " << pulses_received << ") Unexpected number of samples in the RX buffer.";
      cout << " Got: " << n_samps_in_rx_buff << " Expected: " << num_rx_samps << endl;
      cout << "Note: rx_stream->recv can return less than the expected number of samples in some situations, ";
      cout << "but it's not currently supported by this code." << endl;
      cout_mutex.unlock();
      // If you encounter this error, one possible reason is that the buffer sizes set in your transport parameters are too small.
      // For libUSB-based transport, recv_frame_size should be at least the size of num_rx_samps.

      pulses_received++;
      error_count++;
    } else {
      pulses_received++;

      if (chirp.phase_dither) {
        // Undo phase modulation and divide by num_presums in one go
        transform(buff.begin(), buff.end(), buff.begin(), std::bind1st(std::multiplies<complex<float>>(), polar((float) 1.0/chirp.num_presums, inversion_phase)));
      } else if (chirp.num_presums != 1) {
        // Only divide by num_presums
        transform(buff.begin(), buff.end(), buff.begin(), std::bind1st(std::multiplies<complex<float>>(), 1.0/chirp.num_presums));
      }

      // Add to sample_sum
      transform(sample_sum.begin(), sample_sum.end(), buff.begin(), sample_sum.begin(), plus<complex<float>>());
    }

    // Check if we have a full sample_sum ready to write to file
    if (((pulses_received - error_count) > last_pulse_num_written) && ((pulses_received - error_count) % chirp.num_presums == 0)) {
      // As each sample is added, it has phase inversion applied and is divided by # presums, so no additional work to do here.
      // write RX data to file
      if (outfile.is_open()) {
        outfile.write((const char*)&sample_sum.front(), 
          num_rx_samps * sizeof(complex<float>));
      } else {
        cout_mutex.lock();
        cout << "Cannot write to outfile!" << endl;
        cout_mutex.unlock();
        exit(1);
      }
      fill(sample_sum.begin(), sample_sum.end(), complex<float>(0,0)); // Zero out sum for next time
      last_pulse_num_written = pulses_received - error_count;
    }

    // get gps data
    /*if (sdr.clk_ref == "gpsdo" && ((pulses_received % 100000) == 0)) {
      gps_data = usrp->get_mboard_sensor("gps_gprmc").to_pp_string();
      //cout << gps_data << endl;
    }*/

    // check if someone wants to stop
    if (stop_signal_called) {
      cout_mutex.lock();
      cout << "[RX] Reached stop signal handling for outer RX loop -> break" << endl;
      cout_mutex.unlock();
      break;
    }

    // write gps string to file
    /*if (sdr.clk_ref == "gpsdo") {
      boost::asio::async_write(gps_stream, boost::asio::buffer(gps_data + "\n"), gps_asio_handler);
    }*/

    if ( (chirp.max_chirps_per_file > 0) && (int(last_pulse_num_written / chirp.max_chirps_per_file) > save_file_index)) {
      outfile.close();
      // Note: This print statement is used by automated post-processing code. Please be careful about changing the format.
      cout_mutex.lock();
      cout << "[CLOSE FILE] " << current_filename << endl;
      cout_mutex.unlock();

      save_file_index++;
      current_filename = save_loc + "." + to_string(save_file_index);

      // Note: This print statement is used by automated post-processing code. Please be careful about changing the format.
      cout_mutex.lock();
      cout << "[OPEN FILE] " << current_filename << endl;
      cout_mutex.unlock();
      outfile.open(current_filename, ofstream::binary);
    }
    
    // // clear the matrices holding the sums
    // fill(sample_sum.begin(), sample_sum.end(), complex<int16_t>(0,0));
  }

  /*** WRAP UP ***/

  cout << "[RX] Closing output file." << endl;
  outfile.close();
  cout << "[CLOSE FILE] " << current_filename << endl;

  gps_stream.close();

  cout << "[RX] Error count: " << error_count << endl;
  cout << "[RX] Total pulses written: " << last_pulse_num_written << endl;
  cout << "[RX] Total pulses attempted: " << pulses_received << endl;
  
  cout << "[RX] Done. Calling join_all() on transmit thread group." << endl;

  transmit_thread.join_all();

  cout << "[RX] transmit_thread.join_all() complete." << endl << endl;

  return EXIT_SUCCESS;
  
}

/*
 * TRANSMIT_WORKER
 */

void transmit_worker(tx_streamer::sptr& tx_stream, rx_streamer::sptr& rx_stream, Chirp& chirp, Sdr& sdr){
  set_thread_priority_safe(1.0, true);

  // open file to stream from
  ifstream infile("../../" + chirp_loc, ifstream::binary);

  if (!infile.is_open())
  {
    cout << endl
         << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
    cout << "ERROR! Failed to open chirp.bin input file" << endl;
    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl
         << endl;
    exit(1);
  }

  // Transmit buffers

  if (sdr.cpu_format != "fc32") {
    cout << "Only cpu_format 'fc32' is supported for now." << endl;
    // This is because we actually need chirp_unmodulated to have the correct
    // data type to facilitate phase modulation. In the future, this could be
    // fixed up so that it can work with any supported cpu_format, but it
    // seems unnecessary right now.
    exit(1);
  }

  vector<std::complex<float>> tx_buff(num_tx_samps); // Ready-to-transmit samples
  vector<std::complex<float>> chirp_unmodulated(num_tx_samps); // Chirp samples before any phase modulation

  infile.read((char *)&chirp_unmodulated.front(), num_tx_samps * convert::get_bytes_per_item(sdr.cpu_format));
  tx_buff = chirp_unmodulated;

  // Transmit metadata structure
  tx_metadata_t tx_md;
  tx_md.start_of_burst = true;
  tx_md.end_of_burst = true;
  tx_md.has_time_spec = true;

  // Receive command structure
  stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
  stream_cmd.num_samps = num_rx_samps;
  stream_cmd.stream_now = false;

  int chirps_sent = 0;
  double rx_time;
  size_t n_samp_tx;

  long int last_error_count = 0;
  double error_delay = 0;

  while ((chirp.num_pulses < 0) || ((pulses_scheduled - error_count) < chirp.num_pulses))
  {
    // Setup next chirp for modulation
    if (chirp.phase_dither) {
      transform(chirp_unmodulated.begin(), chirp_unmodulated.end(), tx_buff.begin(), std::bind1st(std::multiplies<complex<float>>(), polar((float) 1.0, get_next_phase(true))));
    }

    /*
    The idea here is scheduler a handful of chirps ahead to let
    the transport layer (i.e. libUSB or whatever it is for ethernet)
    buffering actually do its job.
    
    In practice, letting this schedule 10s of pulses ahead seems to
    perform well. According to the documentation, however, the maximum
    queue depth is 8 for both the B20x-mini and X310. (And each pulse
    is two commands -- TX and RX.) So if we're following that, then
    we should only schedule 6 pulses ahead.
    */
    while ((pulses_scheduled - 6) > pulses_received) { // TODO: hardcoded
      if (stop_signal_called) {
        cout << "[TX] stop signal called while scheduler thread waiting -> break" << endl;
        break;
      }
      boost::this_thread::sleep_for(boost::chrono::nanoseconds(10));
    }

    if (error_count > last_error_count) {
      error_delay = (error_count - last_error_count) * 2 * chirp.pulse_rep_int;
      chirp.time_offset += error_delay;
      cout_mutex.lock();
      cout << "[TX] (Chirp " << pulses_scheduled << ") time_offset increased by " << error_delay << endl;
      cout_mutex.unlock();
      last_error_count = error_count;
    }
    // TX
    rx_time = chirp.time_offset + (chirp.pulse_rep_int * pulses_scheduled); // TODO: How do we track timing
    tx_md.time_spec = time_spec_t(rx_time - chirp.tx_lead);
    
    if (sdr.transmit) {
      n_samp_tx = tx_stream->send(&tx_buff.front(), num_tx_samps, tx_md, 60); // TODO: Think about timeout
    }

    // RX
    stream_cmd.time_spec = time_spec_t(rx_time);
    rx_stream->issue_stream_cmd(stream_cmd);

    //cout << "[TX] Scheduled pulse " << pulses_scheduled << " at " << rx_time << " (n_samp_tx = " << n_samp_tx << ")" << endl;

    pulses_scheduled++;

    if (stop_signal_called) {
      cout << "[TX] stop signal called -> break" << endl;
      break;
    }
  }

  cout << "[TX] Closing file" << endl;
  infile.close();
  cout << "[TX] Done." << endl;

}
