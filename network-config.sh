#!/bin/bash

# Read the partial address, start, and stop values from environment variables
PARTIAL_ADDRESS=${WSS_PARTIAL_ADDRESS:-"192.168.1"}
START=${WSS_START_ADDRESS:-2}
STOP=${WSS_END_ADDRESS:-63}

# Add IP addresses to the network interface
for i in $(seq $START $STOP); do
    ip addr add $PARTIAL_ADDRESS.$i/24 dev eth0
done

# Start the application
exec ./start_application.sh
