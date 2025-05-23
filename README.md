# WebSocket Gateway

## intall scyladb

```

docker pull scylladb/scylla

```


Use the commands below

SCYLLA_LAST_NODE defines the number of scyllaDb servers to run
Then run './startScylla.sh'
```
export SCYLLA_LAST_NODE=3 // run 3 servers the default is 5
./startScylla.sh
```

then run the following to see that all containers are up and running

```
docker ps --all
```

then run the following that will set the following environment variables

'''

    WSS_SCYLLA_DB_ADDRESS // the servers ip address in the vm
    WSS_SCYLLADB_PORT     // the servers ports in the vm

'''

```
source ./findScylladbIP.sh
source ./findScyllaDblisteningPorts.sh scylla-node 9042
```



after that use the script findScylladbIP.sh and set the scyla server values

if we have the ip address of the continers we want to run then we can use also

```
   --ip 192.168.3.2 -e SCYLLA_SEEDS="192.168.3.2,192.168.3.3,192.168.3.4"

```

where we need to add the real ip of the node and to support the SCYLLA_SEED to recognize at least the 3 nodes we started (in more nodes running no need to extra ip's to the seed)

```
./findScylladbIP.sh
in the docker run command of the gatway add -e WSS_SCYLLA_DB_ADDRESS="the reult from the script"
or without docker 
export WSS_SCYLLA_DB_ADDRESS="the result from the script"

```


## instalation

```
sudo apt install openssl linux-headers-$(uname -r) libssl-dev net-tools build-essential gcc git wget curl autoconf automake cmake libtool zlib1g-dev docker.io
//to avoid docker permition
sudo groupadd -f docker
sudo usermod -aG docker $USER
newgrp docker

```

## prepare scylladb driver

```
// in projects dir 
// install libuv for scylla drivers
wget https://github.com/libuv/libuv/archive/refs/heads/v1.x.zip
unzip v1.x.zip
cd libuv-1.x
sh autogen.sh
./configure
make
sudo make install
cd ..
rm v1.x.zip
rm -rf libuv-1.x

### install scyllaDb drivers (git clone fails)
wget https://github.com/scylladb/cpp-driver/archive/refs/heads/master.zip
unzip master.zip
cd cpp-driver-master
mkdir build
cd build
cmake ..
make
sudo make install
rm master.zip
rm -rf cpp-driver-master

```

## start OATPP install

```
git clone https://github.com/oatpp/oatpp.git
cd oatpp
mkdir build
cd build
cmake ..
make
sudo make install


```

## start oatpp openssl install

```
git clone https://github.com/oatpp/oatpp-openssl.git
cd oatpp-openssl
mkdir build
cd build
cmake ..
make
sudo make install


```

## create oatpp WebSocket

```
git clone https://github.com/oatpp/oatpp-websocket.git
cd oatpp-websocket
mkdir build
cd build
cmake ..
make
sudo make install

```