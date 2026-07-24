# Initialize the project
AC_INIT([niuchacz], [1.0], [admin@example.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Tell Libtool/Autoconf where to look for local custom M4 macros
AC_CONFIG_MACRO_DIRS([m4])

# Initialize Libtool to handle shared libraries (.so)
LT_INIT

# Check for compilation tools
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Feature: Enable Debug Mode (--enable-debug)
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [Enable debug mode and dev dependencies])],
  [debug_mode=$enableval],
  [debug_mode=no]
)

# Common flags for the entire project (including the requested -msse4.2)
AM_CFLAGS="-m64 -Wall -msse4.2 -D_GNU_SOURCE"
AM_LDFLAGS="-m64 -Wall"

AS_IF([test "x$debug_mode" = "xyes"], [
  # Configuration for 'devbuild'
  AM_CFLAGS="$AM_CFLAGS -g3"
  AM_LDFLAGS="$AM_LDFLAGS -g3"
  AC_SUBST([DEBUG_DEFINES], ["-DDEBUG_MODE"])
], [
  # Configuration for production 'build'
  AM_CFLAGS="$AM_CFLAGS -O3"
])

# Export compiler and linker flags to all Makefiles
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

# Verify mandatory system libraries
AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR([libpthread missing])])
AC_CHECK_LIB([pcap], [pcap_open_live], [], [AC_MSG_ERROR([libpcap missing])])
AC_CHECK_LIB([ssl], [SSL_CTX_new], [], [AC_MSG_ERROR([libssl missing])])
AC_CHECK_LIB([crypto], [EVP_EncryptInit], [], [AC_MSG_ERROR([libcrypto missing])])
AC_CHECK_LIB([config], [config_init], [], [AC_MSG_ERROR([libconfig missing])])

# Define output files (added the Makefile for libalgorithms)
AC_CONFIG_FILES([Makefile libalgorithms/Makefile])
AC_OUTPUT