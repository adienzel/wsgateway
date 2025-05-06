#!/bin/bash

# Default values
DEFAULT_NUM_CONTAINERS=3
DEFAULT_PORT_OFFSET=0
IMAGE_NAME="scylladb/scylla"
NAME_PREFIX="scylla_node"

# Read input or use defaults
NUM_CONTAINERS=${1:-$DEFAULT_NUM_CONTAINERS}
PORT_OFFSET=${2:-$DEFAULT_PORT_OFFSET}

echo "Launching $NUM_CONTAINERS Scylla containers with --network host and dynamic ports..."

for i in $(seq 0 $((NUM_CONTAINERS - 1))); do
  CONTAINER_NAME="${NAME_PREFIX}_${i}"
  OFFSET=$((PORT_OFFSET + i * 100))

  # Assign custom ports per instance
  STORAGE_PORT=$((7000 + OFFSET))
  SSL_STORAGE_PORT=$((7001 + OFFSET))
  NATIVE_PORT=$((9042 + OFFSET))
  RPC_PORT=$((9160 + OFFSET))
  API_PORT=$((10000 + OFFSET))
  PROM_PORT=$((19042 + OFFSET))

  # Create container-specific directories
  DATA_DIR="./data_$i"
  mkdir -p "$DATA_DIR"

  echo "Starting $CONTAINER_NAME on host network with offset $OFFSET"

  docker run -d --name "$CONTAINER_NAME" --network host \
    -v "$(realpath "$DATA_DIR"):/var/lib/scylla" \
    "$IMAGE_NAME" \
    --developer-mode 1 \
    --smp 1 \
    --memory 750M \
    --overprovisioned 1\
    --listen-address 127.0.0.1 \
    --broadcast-address 127.0.0.1 \
    --rpc-address 127.0.0.1 \
    --broadcast-rpc-address 127.0.0.1 \
    --storage-port $STORAGE_PORT \
    --ssl-storage-port $SSL_STORAGE_PORT \
    --native-transport-port $NATIVE_PORT \
    --rpc-port $RPC_PORT \
    --prometheus-address 127.0.0.1 \
    --prometheus-port $PROM_PORT
done
