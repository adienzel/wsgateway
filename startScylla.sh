#!/bin/bash

DEFAULT_NUM_CONTAINERS=3
DEFAULT_PORT_OFFSET=0
IMAGE_NAME="scylladb/scylla"
NAME_PREFIX="scylla_node"

NUM_CONTAINERS=${1:-$DEFAULT_NUM_CONTAINERS}
PORT_OFFSET=${2:-$DEFAULT_PORT_OFFSET}

echo "Starting $NUM_CONTAINERS Scylla containers using --network host with port offset $PORT_OFFSET"

for i in $(seq 0 $((NUM_CONTAINERS - 1))); do
  CONTAINER_NAME="${NAME_PREFIX}_${i}"

  # Offset per container
  OFFSET=$((PORT_OFFSET + i * 100))

  echo "  -> Starting $CONTAINER_NAME with port offset $OFFSET"

  docker run -d --name ${CONTAINER_NAME} --network host \
    -e SCYLLA_API_PORT=$((10000 + OFFSET)) \
    -e SCYLLA_JMX_PORT=$((7199 + OFFSET)) \
    -e SCYLLA_CQL_PORT=$((9042 + OFFSET)) \
    -e SCYLLA_THRIFT_PORT=$((9160 + OFFSET)) \
    -e SCYLLA_STORAGE_PORT=$((7000 + OFFSET)) \
    -e SCYLLA_SSL_STORAGE_PORT=$((7001 + OFFSET)) \
    -e SCYLLA_PROM_PORT=$((19042 + OFFSET)) \
    ${IMAGE_NAME} --smp 1 --memory 750M \
    --api-address 0.0.0.0 \
    --developer-mode 1 \
    --broadcast-address 127.0.0.1 \
    --broadcast-rpc-address 127.0.0.1 \
    --listen-address 127.0.0.1 \
    --rpc-address 127.0.0.1
done
