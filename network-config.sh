#!/bin/bash

# Read the partial address, start, and stop values from environment variables
# removed since started to use ports and not IP address
# PARTIAL_ADDRESS=${WSS_PARTIAL_ADDRESS:-"192.168.1"}
# START=${WSS_START_ADDRESS:-2}
# STOP=${WSS_END_ADDRESS:-63}
# INTERFACE=${WSS_INTERFACE_TO_USE:-"lo"}

# # Add IP addresses to the network interface
# for i in $(seq $START $STOP); do
#     ip addr add $PARTIAL_ADDRESS.$i/24 dev $INTERFACE
# done

# Start the application
exec ./start_application.sh
