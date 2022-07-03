cd build

# zig build-lib \
#   -O Debug \
#   -rdynamic \
#   -dynamic -target wasm32-freestanding ../main.c

 clang \
   --target=wasm32 \
   -O3 \
   -flto \
   -nostdlib \
   -Wl,--no-entry \
   -Wl,--export-all \
   -Wl,--lto-O3 \
   -Wl,--allow-undefined \
   -Wall \
   -mbulk-memory \
   -o main.wasm \
   ../main.c

# ../wasm_sourcemap.py \
#   main.wasm \
#   --dwarfdump /usr/local/opt/llvm/bin/llvm-dwarfdump \
#   -s \
#   -w main.wasm \
#   -u main.wasm.map \
#   -o main.wasm.map
