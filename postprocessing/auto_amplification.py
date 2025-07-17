import sys
import argparse
import numpy as np
import scipy.signal as sp
import processing as pr
import matplotlib.pyplot as plt
from ruamel.yaml import YAML as ym

c = 3e8
pi = 3.14

parser = argparse.ArgumentParser()
parser.add_argument("yaml_file", nargs='?', default='config/default.yaml',help='Path to YAML configuration file')
parser.add_argument("rx_samps_file", nargs = '?', help = 'Path to file with rx data')
args = parser.parse_args()

#initializing variables from yaml
yaml = ym()                         # Always use safe load if not dumping
with open(args.yaml_file) as stream:
   config = yaml.load(stream)

   params = config["RF0"]
   tx_gain = params["tx_gain"]
   rx_gain = params["rx_gain"]
   freq = params["freq"]

   factors = config["ENVIRONMENTAL_FACTORS"]
   
   transmit_power = reference_power + tx_gain - loss
   #TODO: use gps data to calculate radius for geometric spreading
   radius = 100 # should be changed later, this is control data
   wavelength = (c/freq)
   geometric_spreading = ((wavelength)/(4*pi*radius))^2
   scattering_max = factors["scattering_max"]
   scattering_current = factors["scattering_current"]
   specular_reflection_coeff_max = factors["specular_reflection_coeff"]
   specular_reflection_coeff_current = factors["specular_reflection_coeff_current"]
   antenna_gain = factors["antenna_gain"]
   tilt_of_plane = factors["tilt_of_plane"]

print("Config factors and parameters being uploaded...")

bool = True
while(bool):
   #transmit_chirps
   rx_sig = pr.loadSamplesFromFile(args.rx_samps_file, config)
   max_rx_volts = np.max(rx_sig)
   max_rx_samp_power = ((max_rx_volts)^2)/50 #R is 50 ohms

   current_rx_compute_power = transmit_power*antenna_gain*geometric_spreading*scattering_current*specular_reflection_coeff_current #using radar power equation with varibales from config
   max_rx_compute_power = transmit_power*antenna_gain*geometric_spreading*scattering_max*specular_reflection_coeff_max #using radar power equation with varibales from config
   
   dB_transmit_power = 10*np.log10(transmit_power)
   dB_current_power = 10*np.log10(current_rx_compute_power) + 10
   dB_max_power = 10*np.log10(max_rx_compute_power) + 10
   
   db_gain = dB_max_power/dB_transmit_power#calculate gain based on above samples

   if(dB_gain > 76): #76 dB is the max availabe rx gain from b205 mini
      raise ValueError("Calculated rx gain is too high for b205 mini")
   #catch error?

   
   #generate graphs using new gain based on scales power
   #output graphs and new rx_gain

   print("Are you satisfied with this new gain? 'yes' or 'no'")
   user_ans = input().lower()
   if(user_ans = 'yes'):
      bool = False
   else:
      print("Recalculating rx gain")
      bool = True