FROM gcc:latest AS wsgatewaybuild
WORKDIR /usr/src/app

RUN apt-get update && apt-get install -y cmake libssl-dev net-tools wireshark build-essential git curl wget openssl && \
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
FROM alpine:latest

# Install any necessary dependencies (if required by your Go binary)
# RUN apk add --no-cache ca-certificates

RUN mkdir -p /wsgateway
     
# Copy the compiled Go binary from the build stage
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/build/vGateway-exe /wsgateway/vGateway-exe

# Copy the custom network configuration script 
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/network-config.sh /wsgateway/network-config.sh
# Copy the application start script
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/start_application.sh /wsgateway/start_application.sh
# Make the scripts executable 
RUN chmod +x /wsgateway/vGateway-exe && chmod +x /wsgateway/network-config.sh && chmod +x /wsgateway/start_application.sh
# Set the entrypoint to the custom network configuration script 

# Set the working directory in the final image
WORKDIR /wsgateway/

ENV WSS_PARTIAL_ADDRESS="127.0.0"
ENV WSS_START_ADDRESS=3
ENV WSS_END_ADDRESS=63
ENV WSS_PORT="8020"
ENV WSS_HTTP_REQUEST_ADDRESS="127.0.0.1"
ENV WSS_HTTP_REQUEST_PORT="8992"

ENV WSS_NUMBER_OF_WORKER_THREADS=4
ENV WSS_NUMBER_OF_IO_THREADS=4
ENV WSS_NUMBER_OF_TIMER_THREADS=1      

ENV WSS_SCYLLA_DB_ADDRESS="127.0.0.1"
ENV WSS_SCYLLADB_PORT="9060"
ENV WSS_SCYLLADB_KEYSPACE_NAME="vin"
ENV WSS_SCYLLADB_REPLICATION_FACTOR=1
ENV WSS_SCYLLADB_STRATEGY="SimpleStrategy"
ENV WSS_SCYLLADB_TABLE_NAME="vehicles"

RUN apk update && apk add bash iproute2

# ENTRYPOINT [ "/bin/bash" ]
ENTRYPOINT ["./network-config.sh"]

      
      


