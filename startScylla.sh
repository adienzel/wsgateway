#!/bin/bash

# Default values
DEFAULT_NUM_CONTAINERS=3
DEFAULT_PORT_OFFSET=0
IMAGE_NAME="scylladb/scylla"
NAME_PREFIX="scylla_node"

# Read input or use defaults
NUM_CONTAINERS=${1:-$DEFAULT_NUM_CONTAINERS}
PORT_OFFSET=${2:-$DEFAULT_PORT_OFFSET}

echo "Launching $NUM_CONTAINERS Scylla containers with dynamic ports..."

for i in $(seq 0 $((NUM_CONTAINERS - 1))); do
  CONTAINER_NAME="${NAME_PREFIX}_${i}"
#  OFFSET=$((PORT_OFFSET + i * 100))
  OFFSET=$((PORT_OFFSET + i))

  # Assign custom ports per instance (host side)
  HOST_STORAGE_PORT=$((7000 + OFFSET))
  HOST_SSL_STORAGE_PORT=$((7004 + OFFSET))
  HOST_NATIVE_PORT=$((9042 + OFFSET))
  HOST_RPC_PORT=$((9160 + OFFSET))
  HOST_API_PORT=$((10000 + OFFSET))
  HOST_PROM_PORT=$((19042 + OFFSET))

  # Create container-specific directories
  DATA_DIR="./data_$i"
  mkdir -p "$DATA_DIR"

  echo "Starting $CONTAINER_NAME with port offset $OFFSET"

  docker run -d --name "$CONTAINER_NAME" \
    -p ${HOST_STORAGE_PORT}:7000 \
    -p ${HOST_SSL_STORAGE_PORT}:7001 \
    -p ${HOST_NATIVE_PORT}:9042 \
    -p ${HOST_RPC_PORT}:9160 \
    -p ${HOST_API_PORT}:10000 \
    -p ${HOST_PROM_PORT}:9180 \
    -v "$(realpath "$DATA_DIR"):/var/lib/scylla" \
    "$IMAGE_NAME" \
    --developer-mode 1 \
    --smp 1 \
    --memory 750M \
    --overprovisioned 1
done
