import sys
import os
import shutil
import argparse
import numpy as np
import scipy.signal as sp
import processing as pr
import matplotlib.pyplot as plt
from datetime import datetime
from ruamel.yaml import YAML as ym

def save_data(yaml_filename, extra_files={}, alternative_rx_samps_loc=None, num_files=1):
    # Initialize Constants
    yaml = ym()
    with open(yaml_filename) as stream:
        config = yaml.load(stream)

    output_dir = config['FILES'].get('output_dir', 'data')
    save_loc = os.path.join(output_dir, config['FILES']['save_loc'])
    gps_loc = os.path.join(output_dir, config['FILES']['gps_loc'])
    file_prefix = os.path.join(output_dir, datetime.now().strftime("%Y%m%d_%H%M%S"))

    print(f"Copying data to {file_prefix}...")

    shutil.copy(yaml_filename, file_prefix + "_config.yaml")
    if config['FILES']['max_chirps_per_file'] == -1:
            shutil.move(save_loc, file_prefix + "_rx_samps.bin")
    else:
        if config['RUN_MANAGER']['save_partial_files']:
            base_filename = os.path.join(output_dir, config['FILES']['save_loc'])
            for i in range(num_files):
                f = base_filename + "." + str(i)
                shutil.copy(f, file_prefix + "_p" + str(i) + "_rx_samps.bin")
        if alternative_rx_samps_loc is not None:
            shutil.copy(alternative_rx_samps_loc, file_prefix + "_rx_samps.bin")

    for source_file, dest_tag in extra_files.items():
        shutil.copy(source_file, file_prefix + "_" + dest_tag)

    if config['RUN_MANAGER']['save_gps']:
        shutil.copy(gps_loc, file_prefix + "_gps_log.txt")

    print(f"File copying complete.")
    
    return file_prefix

if __name__ == "__main__":
    # Check if a YAML file was provided as a command line argument
    parser = argparse.ArgumentParser()
    parser.add_argument("yaml_file", nargs='?', default='config/default.yaml',
            help='Path to YAML configuration file')
    args = parser.parse_args()
    yaml_filename = args.yaml_file

    save_data(yaml_filename)

