# ****************************************************************************
# * (C) Copyright 2020, Texas Instruments Incorporated. - www.ti.com
# ****************************************************************************
# *
# *  Redistribution and use in source and binary forms, with or without
# *  modification, are permitted provided that the following conditions are
# *  met:
# *
# *    Redistributions of source code must retain the above copyright notice,
# *    this list of conditions and the following disclaimer.
# *
# *    Redistributions in binary form must reproduce the above copyright
# *    notice, this list of conditions and the following disclaimer in the
# *     documentation and/or other materials provided with the distribution.
# *
# *    Neither the name of Texas Instruments Incorporated nor the names of its
# *    contributors may be used to endorse or promote products derived from
# *    this software without specific prior written permission.
# *
# *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# *  PARTICULAR TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# *  A PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  OWNER OR
# *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# *  EXEMPLARY, ORCONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# *  LIABILITY, WHETHER IN CONTRACT,  STRICT LIABILITY, OR TORT (INCLUDING
# *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# *
# ****************************************************************************


# ****************************************************************************
# Sample DCA1000 - mmW Demo LVDS output parser script - should be invoked using python3
#       ex: python3 lvds_example_script.py
#
# Notes:
#   1. User should edit the INPUT CONFIGURATION section below before running the script
#   2. The parser_lvds_demo_captured_file script will output <captured_file>_parser.txt 
#      that contains the text version of the captured files
#   3. This script also outputs the LVDS data shipped in SW session (i.e. detected point cloud)
#      in sw_output.csv to showcase how to use the output of parser_file
#   4. The parser_lvds_demo_captured_file script  will also produce some debug information
#      on the stdio which can be ignored.
# ****************************************************************************

# import parser function
from parser_lvds_demo_captured_file import parser_file
import numpy as np
import matplotlib.pyplot as mpl

##################################################################################
# INPUT CONFIGURATION - TO BE CUSTOMIZED BY USER BEFORE RUNNING SCRIPT
##################################################################################

# Sample values - adopt as per user setup
# Assumes file are in the directory where this script is run
# or PATH to the respective files added as part of the value used here.

# config_file_name: file used to config mmW demo
config_file_name =  'datacard.cfg'
# datacard_prefix: filename prefix provided to DCA1000 CLI;  
datacard_prefix = 'datacard_record'
# numFramesToBePrint: if debug data needs to be printed by the parser script
numFramesToBePrint = 10

##################################################################################
# USE parser_lvds_demo_captured_file.py SCRIPT TO PARSE ABOVE INPUT FILES
##################################################################################
# call the parser API
HW_result, \
HW_numsubframes, \
HW_subframeIdx_buf, \
HW_ADC_buffer, \
HW_CP_buffer, \
HW_CQ1_buffer, \
HW_CQ2_buffer, \
SW_result, \
SW_numsubframes, \
SW_subframeIdx_buf, \
SW_numDetObj, \
x_buffer, \
y_buffer, \
z_buffer, \
v_buffer, \
range_buffer, \
azimuth_buffer, \
elevation_buffer, \
snr_buffer, \
noise_buffer = parser_file(config_file_name, numFramesToBePrint, datacard_prefix)

##################################################################################
# TODO: use the arrays returned by above parser as needed. 
# For array dimensions, see help(parser_file)
# help(parser_file)
##################################################################################

with open("hwout.txt", 'w') as hwfile:
    I = np.zeros(256)
    Q = np.zeros(256)
    i = 0
    q = 0
    maxval = (2**16)/2
    for frame in range(len(HW_ADC_buffer)):
        print(frame)
        ffts = np.zeros((32, 256), dtype=complex)
        for chirp in range(len(HW_ADC_buffer[frame])):
            print(len(HW_ADC_buffer[frame]))
            i = 0
            q = 0
            for data in range(len(HW_ADC_buffer[frame][chirp][0])):
                #if data < 256:
                if data % 2 == 0:
                    I[i] = HW_ADC_buffer[frame][chirp][0][data] /maxval
                    hwfile.write("%f\n" % I[i])
                    i+=1
                else:
                    Q[q] = HW_ADC_buffer[frame][chirp][0][data] /maxval
                    hwfile.write("%f\n" % Q[q])
                    q+=1
            comp = np.multiply(I, 1j*Q)
            ffts[chirp, :] = np.fft.fftshift(np.fft.fft(comp))
        fftm = np.mean(ffts, 0)

    fftm = np.fft.fftshift(fftm)
    mpl.figure(1)
    mpl.plot(I)
    mpl.plot(Q)
    #mpl.xlim([0, ])
    mpl.figure(3)
    mpl.plot(abs(comp))
    mpl.figure(2)
    f = np.fft.fft(comp)
    f = np.fft.fftshift(f)
    mpl.plot(20*np.log10(abs(fftm)))
    mpl.figure(4)
    mpl.plot(abs(fftm)**2)
    mpl.show()