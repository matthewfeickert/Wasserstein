default: arm64

all: arm64

arm64:
	docker pull python:3.10-slim-bullseye
	docker buildx create \
		--name buildx_builder \
		--driver docker-container \
		--bootstrap \
		--use
	docker buildx build \
		--file docker/Dockerfile \
		--platform linux/arm64 \
		--build-arg BUILDER_IMAGE=python:3.10-slim-bullseye \
		--tag ghcr.io/pkomiske/wasserstein:debug-arm64 \
		--load \
		.
	docker buildx stop buildx_builder
	docker buildx rm buildx_builder
