FROM ubuntu as base
WORKDIR /src
RUN apt-get update && apt-get install -y cmake git g++ pkg-config libssl-dev libcurl4-openssl-dev uuid-dev
ENV CBDP_PORT 4242

COPY . .
RUN mkdir cmake-build-debug && cd cmake-build-debug && cmake .. 
RUN cd cmake-build-debug && cmake .. && make worker coordinator

FROM base as coordinator_5
CMD ./cmake-build-debug/coordinator ${CBDP_PORT} ${ACCOUNT} ${TOKEN} ${AZ_CONT} "c" ${N_SUBPART}

FROM base as worker_5
CMD ./cmake-build-debug/worker ${CBDP_COORDINATOR} ${CBDP_PORT} ${ACCOUNT} ${TOKEN} 
