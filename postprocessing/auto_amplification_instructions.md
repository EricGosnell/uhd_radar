## Radar Power Equation Used
**Recieved Power** = `(transmit_power)* (antenna_gain)* (geometric_spreading)* (scattering)* (specular_reflection_coeff)`

*Note: We are assuming that there is no attenutation or polarization mismatch because the surface being flown over is water* 

**transmit_power**(dB) = `reference_power + tx_gain - power_loss` *calculated in code*

**geometric_spreading** = `(wavelength/(4* pi* radius))^2`

**radius**(altitude) = read from gps data, default is 100 meters

**wavelength** = `c/freq`

**c** = `3e8` = speed of light

**freq** = read from default.yaml


*parameters for above equations are specified beloware specified below*

## default.yaml  user input parameters(b205 mini specific)
 **scattering_current:** default is set to `0.5` as the assumed value, but user should change this if needed

 **specular_reflection_coeff_current:** default is set to `0.5` as the assumed value in `s-pol`, but user should change this if needed

 **antenna_gain:** default is set to `6 dB`, but user should change if needed

 **reference_power:** default is set to `___ dB` as control data, but reference_power should be calibrated using........

 **power_loss:** default is set to `___ dB` as control data, but power_loss should be calibrated using .......


## How to convert from linear units to dB

**dB Power** = `10log10(linear_power)` - *That is 10 x log base 10 (linear_power)*

**dB Voltage** = `20log10(linear_voltage)` - *That is 20 x log base 10 (linear_voltage)*
