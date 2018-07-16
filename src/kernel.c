#include "kernel.h"
#include <stdarg.h>
#include <pthread.h>

pthread_mutex_t	gMPP_printfMutex = PTHREAD_MUTEX_INITIALIZER;
int mpp_printf(const char* format, ...) {
  va_list args;
  int ret;

  pthread_mutex_lock(&gMPP_printfMutex);
  va_start(args, format);
  ret = vprintf(format, args);
  va_end(args);
  pthread_mutex_unlock(&gMPP_printfMutex);
  return ret;
}
