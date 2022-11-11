ARG BUILDER_IMAGE=python:3.10-slim-bullseye
FROM ${BUILDER_IMAGE} as builder

ARG TARGETARCH

USER root

SHELL [ "/bin/bash", "-c" ]

COPY . /code
WORKDIR /code

# Set PATH to pickup virtualenv by default
ENV PATH=/usr/local/venv/bin:"${PATH}"
RUN apt-get -qq -y update && \
    apt-get -qq -y install --no-install-recommends \
      gcc \
      g++ \
      make \
      cmake \
      python3-dev \
      git && \
    apt-get -y clean && \
    apt-get -y autoremove && \
    rm -rf /var/lib/apt/lists/* && \
    python -m venv /usr/local/venv && \
    . /usr/local/venv/bin/activate && \
    python -m pip --no-cache-dir install --upgrade \
      pip \
      setuptools \
      wheel && \
    python -m pip --no-cache-dir install . && \
    python -m pip list && \
    printf '\nexport PATH=/usr/local/venv/bin:"${PATH}"\n' >> /root/.bashrc

ENV PYTHONPATH=/usr/local/venv/lib:${PYTHONPATH}
ENV LD_LIBRARY_PATH=/usr/local/venv/lib:${LD_LIBRARY_PATH}

WORKDIR /
