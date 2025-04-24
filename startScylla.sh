#!/bin/bash

START=0
#number scylladb servers = SCYLLA_LAST_NODE + 1 -- 2 means 3 servers
STOP=${SCYLLA_LAST_NODE:-2}

# Add scyllaDB docker nodes
for i in $(seq $START $STOP); do
    PORT=$((i + 2))
    NODE=$((i + 1))
    INT_PORT=$(($i * 2))
    INT_PORT_TLS=$(($INT_PORT + 1))
    TRIFFT_PORT=$((i))
#    echo "NODE = $NODE PORT = 904$PORT INT_PORT = 700$INT_PORT INT_PORT_TLS = 700$INT_PORT_TLS TRIFFT_PORT = 916$TRIFFT_PORT"
    INT_PORT_TLS_PREFIX=700
    output=$(sudo lsof -i :7001)
    if [ -n "$output" ]; then
      INT_PORT_TLS_PREFIX=701
    fi

    if [ "$NODE" == "1" ]; then
      echo "this is node $NODE configuration"
      echo "NODE = $NODE PORT = 904$PORT INT_PORT = 700$INT_PORT INT_PORT_TLS = $INT_PORT_TLS_PREFIX$INT_PORT_TLS TRIFFT_PORT = 916$TRIFFT_PORT"
      docker run --name scylla-node$NODE -p 904$PORT:9042 -p 700$INT_PORT:7000 -p $INT_PORT_TLS_PREFIX$INT_PORT_TLS:7001 -p 916$TRIFFT_PORT:9160 -e SCYLLA_CLUSTER_NAME="ws-cluster" -d scylladb/scylla --smp=4

    else
      echo "this is node - $NODE configuration"
      echo "NODE = $NODE PORT = 904$PORT INT_PORT = 700$INT_PORT INT_PORT_TLS = $INT_PORT_TLS_PREFIX$INT_PORT_TLS TRIFFT_PORT = 916$TRIFFT_PORT"
      docker run --name scylla-node$NODE  -p 904$PORT:9042 -p 700$INT_PORT:7000 -p $INT_PORT_TLS_PREFIX$INT_PORT_TLS:7001 -p 916$TRIFFT_PORT:9160 -e SCYLLA_CLUSTER_NAME="ws-cluster" -d scylladb/scylla -- smp=4 \
              --seeds="$(docker inspect --format='{{ .NetworkSettings.IPAddress }}' scylla-node1)"
    fi
#    docker run --name scylla-node$i --ip 172.17.0.$ADDR -e SCYLLA_SEEDS="172.17.0.2,172.17.0.3,172.17.0.4" -e SCYLLA_CLUSTER_NAME="ws-cluster" -d scylladb/scylla --smp=4

done
