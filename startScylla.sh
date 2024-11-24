#!/bin/bash

START=1
STOP=${SCYLLA_LAST_NODE:-5}

# Add scyllaDB docker nodes
for i in $(seq $START $STOP); do
    ADDR=i+1
    docker run --name scylla-node$i --ip 172.17.0.$ADDR -e SCYLLA_SEEDS="172.17.0.2,172.17.0.3,172.17.0.4" -e SCYLLA_CLUSTER_NAME="ws-cluster" -d scylladb/scylla --smp=4
done
