FROM gcc:latest
WORKDIR /usr/src/app

RUN apt-get update && apt-get install -y cmake libssl-dev net-tools wireshark build-essential git curl wget openssl && \
      git clone https://github.com/oatpp/oatpp.git && cd oatpp && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && \
      make install && cd .. && rm -rf build && cd .. && git clone  https://github.com/oatpp/oatpp-websocket.git && \
      cd oatpp-websocket && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make install && cd .. &&\
      rm -rf build && cd .. 

COPY . .

RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make
