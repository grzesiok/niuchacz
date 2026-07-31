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

# Source definitions for the main application binary service deployment assets
niuchacz_out_SOURCES = \
    src/workermgr/workermgr.c \
    src/psmgr/psmgr.c \
    src/database/database.c \
    src/main.c \
    src/packet_analyze.c

/* FIXED: Replaced raw variables duplicate references to clear native tracking maps variables */
niuchacz_out_LDADD = $(LOCAL_LIBS) $(SYSTEM_LIBS)

# ====================================================================
# SQLITE EMBEDDED SANDBOX AUTOMATED COMPILATION INTRINSICS
# ====================================================================

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

# Clean hook to ensure 'make clean' also wipes the SQLite sandbox
clean-local:
	@if [ -f $(top_srcdir)/sqlite/Makefile ]; then \
		$(MAKE) -C $(top_srcdir)/sqlite clean; \
	fi

# ====================================================================
# CONFIGURATION AND SYSTEMD AUTOMATED NATIVE INSTALLATION PIPELINES
# ====================================================================

# 1. Define configuration delivery paths (maps down to /etc/niuchacz/)
niuchaczconfdir = $(sysconfdir)/niuchacz
niuchaczconf_DATA = sys/etc/niuchacz/niuchacz.conf

# 2. Define Systemd unit configuration targets install directories locations
systemddir = $(prefix)/lib/systemd/system
systemd_DATA = sys/etc/systemd/system/niuchacz.service

# 3. Explicitly include non-code file structures assets into the distribution tarball (make dist)
EXTRA_DIST = \
    sys/etc/niuchacz/niuchacz.conf \
    sys/etc/systemd/system/niuchacz.service

# 4. Hook post-install scripts to ensure state folders exist and manage systemd lifecycle safely
install-data-hook:
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/lib/niuchacz
	chmod 750 $(DESTDIR)$(localstatedir)/lib/niuchacz
	@echo "=== Refreshing systemd unit layout indices ==="
	-systemctl daemon-reload 2>/dev/null || true

# 5. Pre-install lifecycle hooks to safely stop running daemons before binaries updates
install-exec-hook:
	-systemctl stop niuchacz.service 2>/dev/null || true

# 6. Clean deployment files and runtime artifacts during uninstall requests executions
uninstall-hook:
	-systemctl stop niuchacz.service 2>/dev/null || true
	rm -rf $(DESTDIR)$(niuchaczconfdir)
	rm -f $(DESTDIR)$(systemddir)/niuchacz.service
	rm -rf $(DESTDIR)$(localstatedir)/lib/niuchacz
	-systemctl daemon-reload 2>/dev/null || true