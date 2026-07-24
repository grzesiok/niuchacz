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
		./configure $(if $(filter -DDEBUG_MODE,$(DEBUG_DEFINES)),CPPFLAGS=-DSQLITE_DEBUG); \
	fi && \
	$(MAKE) all

# Inject SQLite shared library as a strict build dependency for all targets
BUILT_SOURCES = $(top_builddir)/sqlite/.libs/libsqlite3.so

# Source definitions for the main application binary
niuchacz_out_SOURCES = \
    src/kernel.c \
    src/svc_kernel/psmgr/psmgr.c \
    src/svc_kernel/database/database.c \
    src/svc_kernel/svc_lock.c \
    src/svc_kernel/svc_kernel.c \
    src/svc_kernel/svc_statistics.c \
    src/svc_kernel/svc_update.c \
    src/svc_kernel/execute_unit/cmd_manager.c \
    src/main.c \
    src/import_pcap.c \
    src/export_file.c \
    src/packet_analyze.c \
    src/mapper.c
niuchacz_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

# Source definitions for the application tests
#testhashperf_out_SOURCES = src/kernel.c src/tests/testhashperf.c
#testhashperf_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

#testpsmgr_out_SOURCES = \
#    src/kernel.c \
#    src/tests/testpsmgr.c \
#    src/svc_kernel/database/database.c \
#    src/svc_kernel/svc_lock.c \
#    src/svc_kernel/svc_kernel.c \
#    src/svc_kernel/svc_statistics.c \
#    src/svc_kernel/svc_update.c \
#    src/svc_kernel/execute_unit/cmd_manager.c \
#    src/svc_kernel/psmgr/psmgr.c
#testpsmgr_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

#testcmdmgr_out_SOURCES = \
#    src/kernel.c \
#    src/svc_kernel/psmgr/psmgr.c \
#    src/svc_kernel/database/database.c \
#    src/svc_kernel/svc_lock.c \
#    src/svc_kernel/svc_kernel.c \
#    src/svc_kernel/svc_statistics.c \
#    src/svc_kernel/svc_update.c \
#    src/svc_kernel/execute_unit/cmd_manager.c \
#    src/tests/testcmdmgr.c
#testcmdmgr_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

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