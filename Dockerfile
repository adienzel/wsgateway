FROM gcc:latest AS wsgatewaybuild
WORKDIR /usr/src/app

RUN apt-get update && apt-get install -y cmake libssl-dev net-tools wireshark build-essential git curl wget openssl iproute2 && \
      git clone https://github.com/oatpp/oatpp.git && \
      cd oatpp && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make install && \
      cd .. && rm -rf build && cd .. && \
      git clone https://github.com/oatpp/oatpp-openssl.git && \
      cd oatpp-openssl && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make install && \
      cd .. && rm -rf build && cd .. && \
      git clone  https://github.com/oatpp/oatpp-websocket.git && \
      cd oatpp-websocket && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make install && \
      cd .. && rm -rf build && cd .. && \
      wget https://github.com/libuv/libuv/archive/refs/heads/v1.x.zip && unzip v1.x.zip && cd libuv-1.x && \
      sh autogen.sh && ./configure && make install && cd .. && rm v1.x.zip && rm -rf libuv-1.x && \
      wget https://github.com/scylladb/cpp-driver/archive/refs/heads/master.zip && unzip master.zip && cd cpp-driver-master && \
      mkdir build && cd build && cmake .. && make install && cd .. && rm -rf build && cd ../ && rm master.zip && \
      rm -rf cpp-driver-master

RUN git clone https://github.com/adienzel/wsgateway.git && cd wsgateway && mkdir build && cd build .. && pwd / > /dev/stdout &&  cmake .. && make


# # RUN pwd / > /dev/stdout
# # RUN echo "This is a test message" > /dev/stdout
# # RUN ls -l / > /dev/stdout
# # RUN ls -latr / > /dev/stdout

# RUN  cp /root/app/wsgateway/build/vGateway-exe /root/.
# RUN chmod 644 /usr/src/app/wsgateway/build/vGateway-exe

# RUN ls -l /usr/src/app/wsgateway/build  / > /dev/stdout

# RUN pwd / > /dev/stdout && ls -latr > /dev/stdout && ls -latr /root / > /dev/stdout

# ENTRYPOINT [ "/bin/bash" ]
FROM ubuntu:latest

# Install any necessary dependencies (if required by your Go binary)
# RUN apk add --no-cache ca-certificates

RUN mkdir -p /wsgateway 
     
# Copy the compiled Go binary from the build stage
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/build/vGateway-exe /wsgateway/vGateway-exe

# Copy the custom network configuration script 
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/network-config.sh /wsgateway/network-config.sh
# Copy the application start script
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/start_application.sh /wsgateway/start_application.sh
# copy the find scylladb script
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/findScylladbIP.sh /wsgateway/findScylladbIP.sh
# Make the scripts executable 
RUN chmod +x /wsgateway/vGateway-exe && chmod +x /wsgateway/network-config.sh && chmod +x /wsgateway/start_application.sh && \
      chmod +x /wsgateway/findScylladbIP.sh && \
      apt update && apt install -y net-tools wireshark build-essential git curl wget openssl iproute2
# Set the entrypoint to the custom network configuration script 


#copy all needed libraries 
COPY --from=wsgatewaybuild /usr/local/lib64/libatomic.so.1.2.0 /usr/lib/libatomic.so.1.2.0
COPY --from=wsgatewaybuild /usr/local/lib64/libatomic.so.1 /usr/lib/libatomic.so.1
COPY --from=wsgatewaybuild /usr/local/lib/x86_64-linux-gnu/libscylla-cpp-driver.so.2 /usr/lib/libscylla-cpp-driver.so.2
COPY --from=wsgatewaybuild /usr/local/lib64/libstdc++.so.6.0.33 /usr/lib/libstdc++.so.6.0.33
COPY --from=wsgatewaybuild /usr/local/lib64/libstdc++.so.6 /usr/lib/libstdc++.so
COPY --from=wsgatewaybuild /usr/local/lib/libuv.so.1 /usr/lib/libuv.so.1

# Set the working directory in the final image
WORKDIR /wsgateway/

ENV WSS_ADDRESS="127.0.0.1"
ENV WSS_NUMBER_OF_PORTS=24
ENV WSS_BASE_PORT="8020"
ENV WSS_HTTP_REQUEST_ADDRESS="127.0.0.1"
ENV WSS_HTTP_REQUEST_PORT="8992"

ENV WSS_NUMBER_OF_WORKER_THREADS=4
ENV WSS_NUMBER_OF_IO_THREADS=4
ENV WSS_NUMBER_OF_TIMER_THREADS=1      

# list of server that can be connected
ENV WSS_SCYLLA_DB_ADDRESS="172.17.0.2"
# ENV WSS_SCYLLA_DB_ADDRESS="172.17.0.2,172.17.0.3,172.17.0.4"
ENV WSS_SCYLLADB_PORT="9042"
ENV WSS_SCYLLADB_KEYSPACE_NAME="vin"
ENV WSS_SCYLLADB_REPLICATION_FACTOR=3
# default to SimpleStrategy for testing environment but in real  'NetworkTopologyStrategy' is prefered
ENV WSS_SCYLLADB_STRATEGY="SimpleStrategy" 
ENV WSS_SCYLLADB_TABLE_NAME="vehicles"

#alpine 
# RUN apk update && apk add bash iproute2
# RUN apk add --no-cache --virtual=.build-dependencies wget ca-certificates
# RUN wget -q -O /etc/apk/keys/sgerrand.rsa.pub https://alpine-pkgs.sgerrand.com/sgerrand.rsa.pub
# RUN wget https://github.com/sgerrand/alpine-pkg-glibc/releases/download/2.35-r1/glibc-2.35-r1.apk
# RUN apk add --force-overwrite glibc-2.35-r1.apk
# RUN apk del .build-dependencies 
# RUN rm -rf /var/cache/apk/*

ENTRYPOINT [ "/bin/bash" ]
# ENTRYPOINT ["./network-config.sh"] 
      


