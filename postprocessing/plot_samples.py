from signal import pause
import sys
import argparse
import numpy as np
import scipy.signal as sp
import processing as pr
import matplotlib.pyplot as plt
from ruamel.yaml import YAML as ym


# Check if a YAML file was provided as a command line argument
parser = argparse.ArgumentParser()
parser.add_argument("yaml_file", nargs='?', default='config/default.yaml',
        help='Path to YAML configuration file')
args = parser.parse_args()

# Initialize Constants
yaml = ym()                         # Always use safe load if not dumping
with open(args.yaml_file) as stream:
   config = yaml.load(stream)
   rx_params = config["PLOT"]
   sample_rate = rx_params["sample_rate"]    # Hertz
   rx_samps = rx_params["rx_samps"]          # Received data to analyze
   orig_ch = rx_params["orig_chirp"]         # Chirp associated with the received data
   direct_start = rx_params["direct_start"]
   echo_start = rx_params["echo_start"]
   sig_speed = rx_params["sig_speed"]
   numSamps = rx_params["num_samps"]
   preSamps = rx_params["pre_samps"]
   offsetSamps = rx_params["offset_samps"]

print("--- Loaded constants from config.yaml ---")

# Read and plot RX/TX
rx_sig = pr.extractSig(rx_samps)
print("--- Plotting real samples read from %s ---" % rx_samps)
pr.plotChirpVsTime(rx_sig, 'Received Samples', sample_rate)

tx_sig = pr.extractSig(orig_ch)
print("--- Plotting transmited chirp, stored in %s ---" % orig_ch)
pr.plotChirpVsTime(tx_sig, 'Transmitted Chirp', sample_rate)

# Correlate the two chirps to determine time difference
print("--- Match filtering received chirp with transmitted chirp ---")
xcorr_sig = sp.correlate(rx_sig, tx_sig, mode='valid', method='auto')
# as finddirectpath is written right now, it must be called before taking log of the signal
# because if not, negative log values could have a greater absolute value than positive log values.
dir_peak = pr.findDirectPath(xcorr_sig, direct_start, True) 

#Test code, Only have one option uncommented--------
#Option 1:
xcorr_sig = 20 * np.log10(np.absolute(xcorr_sig))

#Option 2:
#xcorr_normalized = np.abs(xcorr_sig)
#xcorr_normalized /= np.max(xcorr_normalized)
#xcorr_sig = 20 * np.log10(xcorr_normalized + 0.00000000000000001)
#-------------------------------------------

print("--- Plotting result of match filter ---")
xcorr_samps = np.shape(xcorr_sig)[0]
print(echo_start)
xcorr_time = np.zeros(xcorr_samps)
for x in range (xcorr_samps):
    xcorr_time[x] = x * 1e6 / sample_rate

# plt.figure()
# plt.plot(xcorr_time, xcorr_sig)
# plt.title("Output of Match Filter: Signal")
# plt.xlabel('Time (us)')
# plt.ylabel('Power [dB]')
# plt.grid()

#COLIN TEST CODE----------
distPerSample = sig_speed / sample_rate
samples = np.arange(numSamps + preSamps)
xDist = (samples-preSamps) * distPerSample

start = offsetSamps + 5
exclude0Peak = np.argmax(xcorr_sig[start : offsetSamps + numSamps]) + start
localPeak = exclude0Peak - (offsetSamps - preSamps)

print("\tThe direct path peak was found at distance %f with value %f." % (xDist[localPeak], np.abs(xcorr_sig[exclude0Peak])))

plt.figure()
plt.plot(xDist, xcorr_sig[offsetSamps - preSamps : offsetSamps + numSamps])
plt.title("Output of Match Filter: Peaks")
plt.xlabel('Distance (m)')
plt.ylabel('Power [dB]')
plt.plot(xDist[localPeak],xcorr_sig[offsetSamps - preSamps + localPeak])
plt.text(0.95, 0.95,
         f"Peak distance: {xDist[localPeak]:.2f} m",
         transform=plt.gca().transAxes,
         ha='right', va='top')
plt.grid()
#-------------------

#Unedited version of above code
# plt.figure()
# #plt.plot(range(-10,10000), xcorr_sig[dir_peak-10:dir_peak+10000])
# plt.title("Output of Match Filter: Peaks")
# plt.xlabel('Sample')
# plt.ylabel('Power [dB]')
# plt.grid()

[echo_samp, echo_dist] = pr.findEcho(xcorr_sig, sample_rate, dir_peak, echo_start, sig_speed, True)

sys.stdout.flush()
#plt.savefig("match_filter.png", dpi=150)
plt.show()