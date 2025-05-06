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
  CONF_DIR="./conf_$i"
  mkdir -p "$DATA_DIR" "$CONF_DIR"

  # Generate custom scylla.yaml
  SCYLLA_YAML="${CONF_DIR}/scylla.yaml"
  cat > "$SCYLLA_YAML" <<EOF
listen_address: 127.0.0.1
rpc_address: 127.0.0.1
broadcast_address: 127.0.0.1
broadcast_rpc_address: 127.0.0.1

storage_port: $STORAGE_PORT
ssl_storage_port: $SSL_STORAGE_PORT
native_transport_port: $NATIVE_PORT
rpc_port: $RPC_PORT
api_port: $API_PORT
prometheus_port: $PROM_PORT

endpoint_snitch: SimpleSnitch
EOF

  echo "Starting $CONTAINER_NAME on host network with offset $OFFSET"

docker run -d --name "$CONTAINER_NAME" --network host \
  -v "$(realpath "$DATA_DIR"):/var/lib/scylla" \
  -v "$(realpath "$SCYLLA_YAML"):/etc/scylla/custom.yaml:ro" \
  "$IMAGE_NAME" \
  --developer-mode 1 --smp 1 --memory 750M \
  --config-file /etc/scylla/custom.yaml --overprovisioned

#  docker run -d --name "$CONTAINER_NAME" --network host \
#    -v "$(realpath "$DATA_DIR"):/var/lib/scylla" \
#    -v "$(realpath "$SCYLLA_YAML"):/etc/scylla/scylla.yaml:ro" \
#    "$IMAGE_NAME" --smp 1 --memory 750M --developer-mode 1
done
