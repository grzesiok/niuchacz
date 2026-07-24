# Include the local m4 directory for macro scanning
ACLOCAL_AMFLAGS = -I m4

# Specify subdirectories to process first
SUBDIRS = libalgorithms

# Preprocessor flags for the main application
AM_CPPFLAGS = \
    -I$(top_srcdir)/sqlite \
    -I$(top_srcdir)/libalgorithms/include \
    -I$(top_srcdir)/libalgorithms/src \
    -I$(top_srcdir)/src \
    $(DEBUG_DEFINES)

# Link against our new Libtool library and required system libraries
LOCAL_LIBS = $(top_builddir)/libalgorithms/libalgorithms.la
SYSTEM_LIBS = -lpthread -lpcap -lssl -lcrypto -lanl -lconfig

# Programs to build
bin_PROGRAMS = niuchacz.out
noinst_PROGRAMS = testhashperf.out testpsmgr.out testcmdmgr.out

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
testhashperf_out_SOURCES = src/kernel.c src/tests/testhashperf.c
testhashperf_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

testpsmgr_out_SOURCES = \
    src/kernel.c \
    src/tests/testpsmgr.c \
    src/svc_kernel/database/database.c \
    src/svc_kernel/svc_lock.c \
    src/svc_kernel/svc_kernel.c \
    src/svc_kernel/svc_statistics.c \
    src/svc_kernel/svc_update.c \
    src/svc_kernel/execute_unit/cmd_manager.c \
    src/svc_kernel/psmgr/psmgr.c
testpsmgr_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

testcmdmgr_out_SOURCES = \
    src/kernel.c \
    src/svc_kernel/psmgr/psmgr.c \
    src/svc_kernel/database/database.c \
    src/svc_kernel/svc_lock.c \
    src/svc_kernel/svc_kernel.c \
    src/svc_kernel/svc_statistics.c \
    src/svc_kernel/svc_update.c \
    src/svc_kernel/execute_unit/cmd_manager.c \
    src/tests/testcmdmgr.c
testcmdmgr_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

# Automated installation hook for systemd services and deployment assets
install-exec-hook:
	-systemctl stop niuchacz.service
	$(MKDIR_P) $(DESTDIR)/etc/niuchacz
	$(MKDIR_P) $(DESTDIR)$(libdir)
	cp $(top_builddir)/libalgorithms/.libs/libalgorithms.so $(DESTDIR)$(libdir)/libalgorithms.so
	cp $(top_srcdir)/sys/etc/systemd/system/niuchacz.service $(DESTDIR)/etc/systemd/system/niuchacz.service
	cp -r $(top_srcdir)/sys/etc/niuchacz/* $(DESTDIR)/etc/niuchacz/
	-systemctl daemon-reload
	-systemctl start niuchacz.service