#include "yaml-cpp/yaml.h"
#include <iostream>
#include <cstdint>

using namespace std;

class Chirp{

public:
    Chirp(const string& kYamlFile);

    double time_offset; 
    double tx_duration;
    double rx_duration;
    double tr_on_lead;
    double tr_off_trail;
    double pulse_rep_int;
    double tx_lead;
    int num_pulses;
    int num_presums;
    int max_chirps_per_file;
    bool phase_dither;

private: 
    void assignVarFromYaml(const string& kYamlFile);
};
