FROM ubuntu:18.04

# Install common dependencies
RUN apt-get -y update && \
    apt-get -y install sudo \ 
    apt-utils \
    build-essential \
    openssl \
    clang \
    graphviz-dev \
    git \
    libgnutls28-dev \
    psmisc

# Add a new user (namely ubuntu:ubuntu)
RUN groupadd ubuntu && \
    useradd -rm -d /home/ubuntu -s /bin/bash -g ubuntu -G sudo -u 1000 ubuntu -p "$(openssl passwd -1 ubuntu)"

# Use ubuntu as default username
USER ubuntu
WORKDIR /home/ubuntu

# Download and compile AFLNet
ENV LLVM_CONFIG="llvm-config-6.0"
RUN git clone https://github.com/aflnet/aflnet && \
    cd aflnet && \
    make clean all && \
    cd llvm_mode make && make

# Set up environment variables for AFLNet
ENV AFLNET="/home/ubuntu/aflnet"
ENV PATH="${PATH}:${AFLNET}"
ENV AFL_PATH="${AFLNET}"
ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1

# Keep the container running
ENTRYPOINT ["tail", "-f", "/dev/null"]