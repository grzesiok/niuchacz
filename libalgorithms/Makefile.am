# Define the shared library target (LTLIBRARIES)
lib_LTLIBRARIES = libalgorithms.la

# Preprocessor flags dedicated to libalgorithms
AM_CPPFLAGS = \
    -I$(top_srcdir)/libalgorithms/include \
    -I$(top_srcdir)/libalgorithms/src \
    $(DEBUG_DEFINES)

# Library source files (compiled with Libtool)
libalgorithms_la_SOURCES = \
    src/algorithms/stream/cardinality/hyperloglog.c \
    src/ext/murmur/MurmurHash2.c \
    src/ext/murmur/MurmurHash2_64.c \
    src/ext/art/art.c \
    src/algorithms/hash/hash.c \
    src/algorithms/spinlock/spinlock.c \
    src/algorithms/doublylinkedlist/doublylinkedlist.c \
    src/algorithms/queue/queue.c \
    src/algorithms/timer/timer.c \
    src/algorithms/tree/bst.c

# Link the library (Automake adds -shared and -fPIC automatically via Libtool)
libalgorithms_la_LIBADD = -lpthread
libalgorithms_la_LDFLAGS = -version-info 1:0:0

# Test programs to build alongside the library
#noinst_PROGRAMS = testhll.out testdoublylinkedlist.out testqueue.out testbst.out

# Target: testhll.out (Links directly with libalgorithms.la to avoid duplicate compilation)
#testhll_out_SOURCES = tests/testhll.c
#testhll_out_LDADD = libalgorithms.la -lpthread

# Target: testdoublylinkedlist.out
#testdoublylinkedlist_out_SOURCES = tests/testdoublylinkedlist.c
#testdoublylinkedlist_out_LDADD = libalgorithms.la -lpthread

# Target: testqueue.out
#testqueue_out_SOURCES = tests/testqueue.c
#testqueue_out_LDADD = libalgorithms.la -lpthread

# Target: testbst.out
#testbst_out_SOURCES = tests/testbst.c
#testbst_out_LDADD = libalgorithms.la -lpthread