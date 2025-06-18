#include "sdr.h"

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

void Sdr::load_config_from_yaml(const string& kYamlFile) {}
