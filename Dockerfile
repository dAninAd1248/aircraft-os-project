FROM gcc:latest

# Install packages in separate layers. Each layer updates, installs a single package
RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential \
        make \
        git \
        util-linux \
        binutils \
        procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["bash"]
