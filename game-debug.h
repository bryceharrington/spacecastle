#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifndef DEBUG
#define DEBUG (1)
#endif // DEBUG

# define dbg(fmt, ...) \
  do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#endif // __DEBUG_H__
