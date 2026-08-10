FROM ubuntu:24.04@sha256:561618e2c15bf2397621dd04f96926663a3b5616c189cf7e38db7e82f5c538ea

ENV DEBIAN_FRONTEND=noninteractive

ARG TARGETARCH
ARG ARM_GNU_TOOLCHAIN_VERSION=15.3.rel1

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        curl \
        ninja-build \
        python3 \
        python3-pip \
        xz-utils \
    && case "${TARGETARCH}" in \
        amd64) \
            toolchain_host=x86_64; \
            toolchain_sha256=563bebb2b97d53382b956d6ee1fe61e2cae26699901417234a37df505ef9b5fa \
            ;; \
        arm64) \
            toolchain_host=aarch64; \
            toolchain_sha256=06979e0c8171de58e5dc2a2b2019330a290f30930f27728af98a83e1a7369b3a \
            ;; \
        *) \
            echo "Unsupported Docker architecture: ${TARGETARCH}" >&2; \
            exit 1 \
            ;; \
    esac \
    && toolchain_archive="arm-gnu-toolchain-${ARM_GNU_TOOLCHAIN_VERSION}-${toolchain_host}-arm-none-eabi.tar.xz" \
    && toolchain_url="https://gitlab.arm.com/api/v4/projects/tooling%2Fgnu-toolchains-for-arm/packages/generic/gnu-toolchain/${ARM_GNU_TOOLCHAIN_VERSION}/${toolchain_archive}" \
    && curl --fail --location --retry 3 --output "/tmp/${toolchain_archive}" "${toolchain_url}" \
    && echo "${toolchain_sha256}  /tmp/${toolchain_archive}" | sha256sum --check --strict \
    && mkdir -p /opt/arm-gnu-toolchain \
    && tar -xJf "/tmp/${toolchain_archive}" --strip-components=1 -C /opt/arm-gnu-toolchain \
    && rm -f "/tmp/${toolchain_archive}" \
    && python3 -m pip install --break-system-packages intelhex==2.3.0 \
    && rm -rf /var/lib/apt/lists/*

ENV PATH="/opt/arm-gnu-toolchain/bin:${PATH}"

WORKDIR /workspace
COPY Tools /workspace/Tools

RUN chmod +x /workspace/Tools/build_firmware_package.sh

ENTRYPOINT ["/workspace/Tools/build_firmware_package.sh"]
