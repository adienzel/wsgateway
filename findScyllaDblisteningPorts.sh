#!/bin/bash

# Define the container prefix and the internal port you are looking for
prefix=$1
internal_port=$2

# List containers with the given prefix
container_ids=$(docker ps --filter "name=$prefix" --format "{{.ID}}")

# Initialize an empty list to store the external ports
external_ports=""

# Loop through each container and get the external port for the specified internal port
for container_id in $container_ids; do
#  echo "Container ID: $container_id"
  external_port=$(docker inspect $container_id --format="{{(index (index .NetworkSettings.Ports \"$internal_port/tcp\") 0).HostPort}}")
  
  if [ -n "$external_port" ]; then
#    echo "The external port for internal port $internal_port in container $container_id is $external_port"
    # Append the external port to the list
    if [ -z "$external_ports" ]; then
      external_ports="$external_port"
    else
      external_ports="$external_ports,$external_port"
    fi
#  else
#    echo "No external port found for internal port $internal_port in container $container_id"
  fi
done

# Output the comma-delimited list of external ports
#echo "Comma-delimited list of external ports: $external_ports"
#echo "$external_ports"
export WSS_SCYLLADB_PORT="$external_ports"