# Define the shared library target (LTLIBRARIES)
lib_LTLIBRARIES = libalgorithms.la

# Preprocessor flags dedicated to libalgorithms - added explicit subdirectories
AM_CPPFLAGS = \
    -I$(top_srcdir)/libalgorithms/include \
    -I$(top_srcdir)/libalgorithms/src \
    -I$(top_srcdir)/libalgorithms/src/algorithms/queue \
    -I$(top_srcdir)/libalgorithms/src/algorithms/spinlock \
    -I$(top_srcdir)/libalgorithms/src/algorithms/doublylinkedlist \
    -I$(top_srcdir)/libalgorithms/src/algorithms/timer \
    -I$(top_srcdir)/libalgorithms/src/algorithms/tree \
    -I$(top_srcdir)/libalgorithms/src/algorithms/hash \
    -I$(top_srcdir)/libalgorithms/src/algorithms/stream/cardinality \
    -I$(top_srcdir)/libalgorithms/src/ext/art \
    -I$(top_srcdir)/libalgorithms/src/ext/murmur \
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

# ====================================================================
# UNIT TESTING INTEGRATION (CHECK FRAMEWORK)
# ====================================================================

# Specify test runners executed via 'make check' command execution
TESTS = test_queue.out test_bst.out
check_PROGRAMS = test_queue.out test_bst.out

# Source definitions and compilation flags for the queue test binary
test_queue_out_SOURCES = tests/test_queue.c
test_queue_out_CFLAGS = $(AM_CFLAGS) $(CHECK_CFLAGS)
test_queue_out_LDADD = libalgorithms.la $(CHECK_LIBS) -lpthread

# Source definitions and compilation flags for the BST tree test binary
test_bst_out_SOURCES = tests/test_bst.c
test_bst_out_CFLAGS = $(AM_CFLAGS) $(CHECK_CFLAGS)
test_bst_out_LDADD = libalgorithms.la $(CHECK_LIBS) -lpthread