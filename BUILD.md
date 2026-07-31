# Generate the updated build infrastructure (incorporating Libtool)
autoreconf --install

or

autoreconf -fi

## Option A: Production Build (Optimized profile)
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
make

## Option B: Development Build (-g3 and DEBUG_MODE profile)
./configure --enable-debug --prefix=/usr --sysconfdir=/etc --localstatedir=/var
make

# Run Unit tests
make check

or

make check VERBOSE=1