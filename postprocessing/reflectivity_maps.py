import numpy as np
import sys
import argparse
import pandas as pd
import scipy.signal as sp
import processing as pr
import seaborn as sns
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
from scipy.signal import find_peaks
from scipy.interpolate import griddata
from ruamel.yaml import YAML as ym

#installed seaborn and scipy with 'conda install seaborn scipy'

"""
This script processes radar data to analyze and visualize peak returns.

Workflow:
- Loads and plots the transmitted chirp and received samples (from `plot_samples.py`)
- Applies matched filtering between the transmitted and received signals
- Uses `scipy.signal.find_peaks` to identify prominent reflection peaks
- Plots:
    • Matched filter output
    • Peak power vs. time
    • Interpolated 2D scatter plot (voltage vs. lat/lon)
    • 3D scatter plot of voltage over lat/lon/altitude
    • (Optional) 3D surface plot of interpolated data (currently commented out)
"""

# check if Yaml was provided as a command line argument
parser = argparse.ArgumentParser()
parser.add_argument("yaml_file", nargs = '?', default = 'config/default.yaml', #may need to navigate out and to config
                    help='Path to YAML configuration file')
args = parser.parse_args()

#initialize Constants
yaml = ym()  # Always use safe load if not dumping
with open(args.yaml_file) as stream:
    config = yaml.load(stream)
    rx_params = config["PLOT"]
    generate = config["GENERATE"]
    files = config["FILES"]
    sample_rate = rx_params["sample_rate"]  # Hertz
    rx_samps = rx_params["rx_samps"]        # Received data to analyze
    orig_ch = rx_params["orig_chirp"]       # Chirp associated with the received data
    pulse_length = generate["pulse_length"]  # Length of the pulse in seconds
    chirp_length = generate["chirp_length"]  # Length of the chirp in seconds
    gps_loc = files["gps_loc"]  # GPS location file 

print("--- Loaded constants from config.yaml ---")

#read and plot RX/TX
rx_sig = pr.extractSig(rx_samps)
print("--- Plotting real samples read from %s ---" % rx_samps)
pr.plotChirpVsTime(rx_sig, 'Received Samples', sample_rate)

tx_sig = pr.extractSig(orig_ch)
print("--- Plotting transmited chirp, stored in %s ---" % orig_ch)
pr.plotChirpVsTime(tx_sig, 'Transmitted Chirp', sample_rate)

# Correlate the two chirps to determine time difference
print("--- Match filtering received chirp with transmitted chirp ---")
xcorr_sig = sp.correlate(rx_sig, tx_sig, mode='full', method='auto')
xcorr_sig = 20 * np.log10(np.absolute(xcorr_sig))

print("--- Plotting result of match filter ---")
lags = np.arange(-len(tx_sig) + 1, len(rx_sig))
xcorr_time = lags * 1e6 / sample_rate

#peak finding using scipy
plt.figure()
peaks, _ = find_peaks(xcorr_sig, distance= sample_rate * pulse_length)
np.diff(peaks)
plt.plot(xcorr_time, xcorr_sig)
plt.plot(xcorr_time[peaks],xcorr_sig[peaks], "x")
plt.title("Output of Match Filter: Signal")
plt.xlabel('Time (us)')
plt.ylabel('Power [dB]')
plt.grid()

time_peaks = xcorr_time[peaks]
voltageDB = xcorr_sig[peaks]
#convert chirp amplitude to power
#assuming 50 ohm impedance from b205mini, convert dB to power
voltageDB = (voltageDB**2)/(50*chirp_length)    #**confused on this conversion ask nicole

plt.figure()
plt.plot(time_peaks, voltageDB, "o")
plt.xlabel('Time (us)')
plt.ylabel('Power [dB]')
plt.title("Peak Power (dB) vs Time")
plt.grid()

# TODO: GPS is good at walking pace, PLane goes faster and may not get pretty data on graph
# must think about the speed of the plane and the distance travelled by the transmission

# Setting up GPS data for heatmap from 
gps_time, lat, lon, alt = pr.loadGPSData(gps_loc)  # Load GPS data

#TODO: alt will be of the plane, we must take plane alt - distance travelled by transmission to get ground alt

gps_time = gps_time - gps_time[0]  # Normalize GPS time to start at 0
gps_time = gps_time/1e6  # Convert GPS time to seconds for consistency with xcorr_time
# Interpolating the peak voltage and gps info
# First we must create a common timebase, where we want to see data on the graph at
start_time = max(min(gps_time), min(time_peaks))
end_time = min(max(gps_time), max(time_peaks))

time_step = 0.025 # 25 ms step, adjust as needed. maybe add in a different place for config

time_base = np.arange(start_time, end_time, time_step)

print("start_time:", start_time)
print("end_time:", end_time)

# Interpolating the peak voltage and gps info to the common time base
voltage_interp = np.interp(time_base, time_peaks, voltageDB)  # Interpolating peak voltage to common time base
lat_interp = np.interp(time_base, gps_time, lat)  # Interpolating latitude to common time base
lon_interp = np.interp(time_base, gps_time, lon)  # Interpolating longitude to common time base
alt_interp = np.interp(time_base, gps_time, alt)  # Interpolating altitude to common time base

# 2D scatter plot of the interpolated data
plt.figure()

interp_scatter = plt.scatter(x=lon_interp, y=lat_interp, c=voltage_interp, cmap="BuPu", s=50, edgecolors='none')

cbar = plt.colorbar(interp_scatter)
cbar.set_label("Voltage (dB)")
plt.title("Interpolated Scatterplot: Voltage by Location")
plt.xlabel("Longitude")
plt.ylabel("Latitude")

ax = plt.gca()
ax.get_xaxis().get_major_formatter().set_useOffset(False)
ax.get_yaxis().get_major_formatter().set_useOffset(False)
ax.tick_params(axis='both', which='major', labelsize=7)

plt.grid(True)

"""
#Interpolating the altitude and voltage to create a 3D surface plot
#Uncomment this code to visualize the 3D surface plot, it takes a lot of processing time
lon_grid, lat_grid = np.meshgrid(lon_interp, lat_interp)  # Create a meshgrid for latitude and longitude

alt_grid = griddata((lon_interp, lat_interp), alt_interp, (lon_grid, lat_grid), method='linear')  # Interpolate altitude
voltage_grid = griddata((lon_interp, lat_interp), voltage_interp, (lon_grid, lat_grid), method='linear')  # Interpolate voltage

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.plot_surface(lon_grid, lat_grid, alt_grid, facecolors=cm.BuPu(voltage_grid / np.nanmax(voltage_grid)), rstride=1, cstride=1, antialiased=True)
ax.view_init(elev=20, azim=-60)  #view angle
ax.set_xlabel('Longitude')
ax.set_ylabel('Latitude')
ax.set_zlabel('Altitude (m)')
ax.set_title('Voltage(dB) by Location and Altitude')

ax.get_xaxis().get_major_formatter().set_useOffset(False)
ax.get_yaxis().get_major_formatter().set_useOffset(False)
ax.tick_params(axis='both', which='major', labelsize=7)
plt.show()
"""
# 3D scatter plot of the interpolated data
#TODO: plots path altitude, we need altitude of the ground
#there are problems with distance data as of 07/31/2025 and not enought time to resolve
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
sc = ax.scatter(
    lon_interp, lat_interp, alt_interp,
    c=voltage_interp,
    cmap='BuPu',
    s=30
)

ax.set_xlabel('Longitude')
ax.set_ylabel('Latitude')
ax.set_zlabel('Altitude (m)')
ax.set_title('3D Scatterplot: Voltage by Location and Altitude')

ax.get_xaxis().get_major_formatter().set_useOffset(False)
ax.get_yaxis().get_major_formatter().set_useOffset(False)
ax.tick_params(axis='both', which='major', labelsize=7)

cbar = fig.colorbar(sc, ax=ax, pad=0.1)
cbar.set_label('Voltage (dB)')

plt.show()

#git commit -m "feat(postprocessing->reflectivity): Added peak finding and base reflectivity map functions" -m"Added new file called reflectivity_maps.py which makes graphs 2D and 3D of Power dB