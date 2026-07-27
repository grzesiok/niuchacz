# Only process subdirectories that use our Autotools config
SUBDIRS = libalgorithms

# Preprocessor flags for the main application
AM_CPPFLAGS = \
    -I$(top_srcdir)/sqlite \
    -I$(top_srcdir)/libalgorithms/include \
    -I$(top_srcdir)/libalgorithms/src \
    -I$(top_srcdir)/src \
    $(DEBUG_DEFINES)

# Path to the shared libraries (SQLite is referenced at its native build location)
LOCAL_LIBS = \
    $(top_builddir)/sqlite/.libs/libsqlite3.so \
    $(top_builddir)/libalgorithms/libalgorithms.la

SYSTEM_LIBS = -lpthread -lpcap -lssl -lcrypto -lconfig

# Programs to build
bin_PROGRAMS = niuchacz.out
#noinst_PROGRAMS = testhashperf.out testpsmgr.out testcmdmgr.out

# Rule to build the standalone SQLite folder using its own configure system
$(top_builddir)/sqlite/.libs/libsqlite3.so:
	@echo "=== Building standalone SQLite component ==="
	cd $(top_srcdir)/sqlite && \
	if [ ! -f Makefile ]; then \
		./configure $(if $(filter -DDEBUG_MODE,$(DEBUG_DEFINES)),CPPFLAGS="-DSQLITE_DEBUG" CFLAGS="-g -O2 -Wno-implicit-function-declaration"); \
	fi && \
	$(MAKE) all

# Inject SQLite shared library as a strict build dependency for all targets
BUILT_SOURCES = $(top_builddir)/sqlite/.libs/libsqlite3.so

# Source definitions for the main application binary service deployment assets
niuchacz_out_SOURCES = \
    src/workermgr/workermgr.c \
    src/psmgr/psmgr.c \
    src/database/database.c \
    src/main.c
niuchacz_out_LDADD = $(top_builddir)/libalgorithms/libalgorithms.la $(top_builddir)/sqlite/.libs/libsqlite3.so -lpthread -lpcap -lssl -lcrypto -lconfig

# Clean hook to ensure 'make clean' also wipes the SQLite sandbox
clean-local:
	@if [ -f $(top_srcdir)/sqlite/Makefile ]; then \
		$(MAKE) -C $(top_srcdir)/sqlite clean; \
	fi

# Automated installation hook for systemd services and deployment assets
install-exec-hook:
	-systemctl stop niuchacz.service
	$(MKDIR_P) $(DESTDIR)/etc/niuchacz
	$(MKDIR_P) $(DESTDIR)$(libdir)
	cp $(top_builddir)/sqlite/.libs/libsqlite3.so $(DESTDIR)$(libdir)/libsqlite3.so
	cp $(top_builddir)/libalgorithms/.libs/libalgorithms.so $(DESTDIR)$(libdir)/libalgorithms.so
	cp $(top_srcdir)/sys/etc/systemd/system/niuchacz.service $(DESTDIR)/etc/systemd/system/niuchacz.service
	cp -r $(top_srcdir)/sys/etc/niuchacz/* $(DESTDIR)/etc/niuchacz/
	-systemctl daemon-reload
	-systemctl start niuchacz.service