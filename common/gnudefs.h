#ifndef GNUDEFS_H
#define GNUDEFS_H

#ifdef __GNUC__
# define GCC_PRINTF_ATTRIB(x, y) __attribute__((format(printf, x, y)));
#else
# define GCC_PRINTF_ATTRIB(x)
#endif

#endif
