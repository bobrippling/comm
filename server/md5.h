#ifndef MD5_H
#define MD5_H

char *md5(const char *);

/* 0 on correct password */
int md5check(const char *, const char *);

#endif
