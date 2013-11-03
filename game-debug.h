#ifndef __DEBUG_H__
#define __DEBUG_H__
#ifdef DEBUG

#ifdef dbg
# define dbg(msg) printf(msg);

#else // DEBUG
# define dbg(msg)
#endif

#endif // DEBUG
#endif // __DEBUG_H__
