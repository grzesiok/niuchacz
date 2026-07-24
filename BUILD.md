# Generate the updated build infrastructure (incorporating Libtool)
autoreconf --install

# Option A: Production Build (Optimized profile)
./configure
make

# Option B: Development Build (-g3 and DEBUG_MODE profile)
./configure --enable-debug
make