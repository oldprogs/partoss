#include <unistd.h>
#ifdef __EMX__
#include <sys/types.h>
#endif
#include <dirent.h>

#include "_dos.h"

#ifndef __EMX__
#define O_BINARY 0
#endif

#define P_WAIT 0

#define strnicmp strncasecmp
#define strcmpi strcasecmp
#define stricmp strcasecmp
#define memicmp bcmp

#define cprintf printf

#define SH_DENYNO 0666
#define SH_DENYRW 0666
#define SH_DENYWR 0666
#define sopen open

#ifndef __EMX__
extern char *strupr (char *__string);
extern char *strlwr (char *__string);
#endif
extern long filelength (int __handle);
extern int spawnvp (int __mode, const char *__path, char *const *__argv);

extern int chsize (int __handle, long __size);


extern unsigned _dos_findfirst (const char *__path, unsigned __attr,
				struct find_t *__buf);
extern unsigned _dos_findnext (struct find_t *__buf);
extern unsigned _dos_findclose (struct find_t *__buf);
extern char *itoa (int __value, char *__buf, int __radix);
extern void _dos_getdrive (unsigned *__drive);
extern void _dos_setdrive (unsigned __drivenum, unsigned *__drives);
extern int mkdir (const char *__path);
extern unsigned _dos_getftime (int __handle, unsigned short *__date,
			       unsigned short *__time);
