cd build

zig build-lib \
  -rdynamic \
  -dynamic -target wasm32-freestanding ../main.c
