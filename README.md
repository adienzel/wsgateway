# WebSocket Gateway

## reqired
intall scyladb
,,,
docker pull scylladb/scylla
docker run --name scylla-node1 --cpus=2 -d scylladb/scylla
docker run --name scylla-node2 --cpus=2 -d scylladb/scylla
docker run --name scylla-node3 --cpus=2 -d scylladb/scylla

,,,

after that use the script findScylladbIP.sh and set the scyla server values

```
./findScylladbIP.sh
in the docker run command of the gatway add -e WSS_SCYLLA_DB_ADDRESS="the reult from the script"
or without docker 
export WSS_SCYLLA_DB_ADDRESS="the reult from the script"

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