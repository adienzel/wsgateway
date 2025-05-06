#!/bin/bash
nodes=$(docker ps --all | grep scylla_node | awk '{print $NF}')nodes=$(docker ps --all | grep scylla_node | awk '{print $NF}')

# Extract the prefix numbers and sort them 
prefix_numbers=$(echo "$nodes" | grep -oP '(?<=scylla_node_)\d+' | sort -n)
# Find the lowest and highest numbers 
START=$(echo "$prefix_numbers" | head -n 1) 
STOP=$(echo "$prefix_numbers" | tail -n 1)

# START=${FIRST_NODE_NUMBER:-1}
# STOP=${LAST_NODE_NUMBER:-3}

ip_addresses=()
for i in $(seq $START $STOP); do
   ip=$(docker inspect scylla_node_${i} | grep -v "SecondaryIPAddresses" | grep -oP '(?<="IPAddress": ")[^"]+') 
   ip_addresses+=($ip)
#    docker inspect scylla_node${i} | grep -v "SecondaryIPAddresses" | grep "IPAddress" | sort | uniq
done

unique_ips=$(echo "${ip_addresses[@]}" | tr ' ' '\n' | sort -u | tr '\n' ',' | sed 's/,$//')

echo $unique_ips
export WSS_SCYLLA_DB_ADDRESS="$unique_ips"