# ---- Build stage --------------------------------------------------------
# Emscripten SDK pinned to the version used by ci-build.sh.
FROM emscripten/emsdk:3.1.25 AS build

# Host toolchain needed to build MicroPython's build tools (mpy-cross,
# qstr generation, etc.) and to run make.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        git \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Install JS dependencies first. The Makefile only needs esbuild (invoked via
# `npx esbuild`). The committed package-lock.json is lockfileVersion 3, which
# the npm 6 bundled in this emsdk image cannot read, so install esbuild
# directly at the pinned version instead of using `npm ci`.
COPY package.json ./
RUN npm install --no-save esbuild@0.14.49

# Copy the rest of the sources (submodules are expected to be checked out
# on the host; see .dockerignore which excludes build artifacts only).
COPY . .

# Build the WASM firmware + simulator JS and assemble the build/ directory.
RUN make

# ---- Serve stage --------------------------------------------------------
FROM nginx:1.27-alpine AS serve

# Static simulator output produced by `make`.
COPY --from=build /app/build /usr/share/nginx/html

# Cross-origin isolation headers + correct wasm mime handling.
COPY docker/nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80
