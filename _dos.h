#ifndef __DOS_H_
#define __DOS_H_
#include <dirent.h>
#include <sys/types.h>

#define _A_NORMAL       0x00	/* Normal file - read/write permitted */
#define _A_RDONLY       0x01	/* Read-only file */
#define _A_HIDDEN       0x02	/* Hidden file */
#define _A_SYSTEM       0x04	/* System file */
#define _A_VOLID        0x08	/* Volume-ID entry */
#define _A_SUBDIR       0x10	/* Subdirectory */
#define _A_ARCH         0x20	/* Archive file */


struct find_t
{
  char reserved[21];		/* reserved for use by DOS    */
  char attrib;			/* attribute byte for file    */
  unsigned short wr_time;	/* time of last write to file */
  unsigned short wr_date;	/* date of last write to file */
  unsigned long size;		/* length of file in bytes    */
#if defined(__OS2__) || defined(__NT__) || defined(__LNX__) || defined(__EMX__)
  char name[256];		/* null-terminated filename   */
#elif defined (__FreeBSD__)
  char name[MAXNAMLEN+1];
#else
  char name[13];		/* null-terminated filename   */
#endif
#if defined (__LNX__) || defined (__FreeBSD__)
  DIR *__entry;
  char *__pattern;
  char *__path_only;
  off_t dir_pos;
#endif
};

#define _find_t find_t
#endif
