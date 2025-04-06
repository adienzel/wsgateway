#!/bin/bash

# Prefix for the container names
PREFIX="your_prefix"

# Stop containers with the specified prefix
docker ps --filter "name=${PREFIX}" -q | xargs -r docker stop

# Remove containers with the specified prefix
docker ps -a --filter "name=${PREFIX}" -q | xargs -r docker rm
