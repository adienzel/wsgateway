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
      rm -rf cpp-driver-master && \
      git clone https://github.com/adienzel/wsgateway.git && cd wsgateway && mkdir build && cd build .. && pwd / > /dev/stdout &&  cmake .. && make


# # RUN pwd / > /dev/stdout
# # RUN echo "This is a test message" > /dev/stdout
# # RUN ls -l / > /dev/stdout
# # RUN ls -latr / > /dev/stdout

# RUN  cp /root/app/wsgateway/build/vGateway-exe /root/.


# RUN pwd / > /dev/stdout && ls -latr > /dev/stdout && ls -latr /root / > /dev/stdout


FROM alpine:latest

# Install any necessary dependencies (if required by your Go binary)
# RUN apk add --no-cache ca-certificates
      
# Set the working directory in the final image
WORKDIR /root/

# RUN pwd / > /dev/stdout && ls -latr / > /dev/stdout

# Copy the compiled Go binary from the build stage
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/build/vGateway-exe /usr/local/bin/.

# Copy the custom network configuration script 
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/configure_network.sh /usr/local/bin/configure_network.sh
# Copy the application start script
COPY --from=wsgatewaybuild /usr/src/app/wsgateway/start_application.sh /usr/local/bin/start_application.sh
# Make the scripts executable 
RUN chmod +x /usr/local/bin/vGateway-exe && chmod +x /usr/local/bin/configure_network.sh && chmod +x /usr/local/bin/start_application.sh
# Set the entrypoint to the custom network configuration script 

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


ENTRYPOINT ["/usr/local/bin/configure_network.sh"]
      
      
      


