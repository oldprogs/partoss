#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <time.h>
#include <sys\stat.h>
#include <ctype.h>
#include <errno.h>

struct dostime {
  unsigned day,
    month,
    year,
    sec,
    min,
    hour;
};

int hex(char hexdigit);
int getstring(void);
int gettoken(void);
int readblock(int fhandle);
unsigned long crc32block(unsigned char *ptr, int count, unsigned long tcrc =
			 0);
unsigned long hash(char *f);
unsigned long strtime(char *time);
unsigned long updatecrc32(unsigned long crc, char c);
void converttime(char *time);
void hexascii(unsigned long value, char *string);
void tokencpy(char *string);
void logwrite(int first);
unsigned long asciihex(char *string);

char *string,
 *token,
  sbuffer[256],
 *tstrtime,
  ftstime[20];
char *duotrice = "0123456789abcdefghijklmnopqrstuv";
char *months[12] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
  "Nov", "Dec"
};
int packset,
  key,
  logfile,
  toklen,
  maxstr,
  maxlen,
  curspos,
  endinput,
  endblock,
  endstring,
  curtpos;
unsigned long firstkey,
  secondkey;
unsigned long namehash,
  namecrc,
  addrhash,
  addrcrc;
char regname[80],
  regsaddr[80],
  logout[256];
time_t sftime;
struct tm *tmt;

unsigned long far cr3tab[256] = {	/* CRC polynomial 0xedb88320 */
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd,
  0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb,
  0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447,
  0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75,
  0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11,
  0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
  0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b,
  0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49,
  0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
  0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3,
  0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af,
  0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d,
  0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9,
  0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767,
  0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703,
  0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
  0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d,
  0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b,
  0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
  0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5,
  0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1,
  0x5a05df1b, 0x2d02ef8d
};

int main(void)
{
  packset = open("partoss.reg", O_RDONLY | O_BINARY);
  if(packset == -1)
    return 1;
  maxlen = readblock(packset);
  maxstr = getstring();
  toklen = gettoken();
  memset(regname, 0, 80);
  tokencpy(regname);
  maxstr = getstring();
  toklen = gettoken();
  memset(regsaddr, 0, 80);
  tokencpy(regsaddr);
  close(packset);
  key = open("partoss.key", O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
  if(key == -1)
    return 2;
  logfile = open("partreg.log", O_RDWR | O_BINARY);
  if(logfile == -1)
  {
    logfile =
      open("partreg.log", O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if(logfile == -1)
      return 3;
  }
  lseek(logfile, 0, SEEK_END);
  namehash = hash(regname);
  addrhash = hash(regsaddr);
  namecrc = crc32block(regname, strlen(regname));
  addrcrc = crc32block(regsaddr, strlen(regsaddr));
  firstkey = namecrc ^ addrhash;
  secondkey = namehash ^ addrcrc;

  memset(logout, 0, 128);
  hexascii(firstkey, logout);
  hexascii(secondkey, logout + 8);
  strcat(logout, "\r\n");
  strcat(logout, regname);
  strcat(logout, "\r\n");
  strcat(logout, regsaddr);
  strcat(logout, "\r\n");
  write(key, logout, strlen(logout));
  close(key);
  sprintf(logout, "Registration for %s (%s)\r\n", regname, regsaddr);
  ccprintf("%s", logout);
  logwrite(1);
  close(logfile);
  return 0;
}

int hex(char hexdigit)
{
  switch (hexdigit)
  {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return hexdigit - '0';
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      return hexdigit - 'A' + 10;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
      return hexdigit - 'a' + 10;
  }
  return 0;
}

unsigned long asciihex(char *string)
{
  unsigned int i,
    j;
  unsigned long k = 0;

  for(i = 0; i < 8; i++)
  {
    k <<= 4;
    j = hex(string[i]);
    k += j;
  }
  return k;
}

int readblock(int fhandle)
{
  int i,
    j = 0,
    bsize;

  bsize = read(fhandle, sbuffer, 256);
  curspos = 0;
  if(bsize < 256)
    endinput = 1;
  else
  {
    for(i = 255; (i >= 0) && (sbuffer[i] != '\n') && (sbuffer[i] != '\r');
	i--, j++) ;
    lseek(fhandle, -j, SEEK_CUR);
    bsize = i;
    endinput = 0;
  }
  return bsize;
}

int getstring(void)
{
  int i,
    j;

  curtpos = 0;
  string = sbuffer + curspos;
  while(isspace(*string))
    string++, curspos++;
  i = curspos;
  while((i < maxlen) && (sbuffer[i] != '\n') && (sbuffer[i] != '\r'))
    i++;
  if(i < maxlen)
    while((i < maxlen) && ((sbuffer[i] == '\n') || (sbuffer[i] == '\r')))
      i++;
  j = i - curspos;
  curspos = i;
  if(curspos >= maxlen)
    endblock = 1;
  return j;
}

int gettoken(void)
{
  int i,
    j;

  for(i = curtpos; (i < maxstr) && isspace(string[i]); i++) ;
  if(string[i] == '"')
    for(j = ++i; (j < maxstr) && string[j] != '"'; j++) ;
  else
    for(j = i; (j < maxstr) && !isspace(string[j]); j++) ;
  token = string + i;
  curtpos = j + 1;
  if(curtpos >= maxstr || string[j] == '\r' || string[j] == '\n')
    endstring = 1;
  return j - i;
}

void tokencpy(char *string)
{
  memcpy(string, token, toklen);
  string[toklen] = 0;
}

unsigned long hash(char *string)
{
  unsigned long result = 0,
    temp;
  char *ttemp;

  for(ttemp = string; *ttemp; ttemp++)
  {
    result = (result << 4) + tolower(*ttemp);
    if((temp = (result & 0xf0000000L)) != 0L)
    {
      result |= temp >> 24;
      result |= temp;
    }
  }
  return (result & 0x7fffffffLu);
}

void logwrite(int first)
{
  if(first)
  {
    sftime = time(NULL);
    tmt = localtime(&sftime);
    tstrtime = asctime(tmt);
    converttime(tstrtime);
    ftstime[19] = ' ';
    write(logfile, ftstime, 20);
    ftstime[19] = 0;
  }
  write(logfile, logout, strlen(logout));
}

void hexascii(unsigned long value, char *string)
{
  char tt,
    i,
    j;

  for(i = 0; i < 4; i++)
  {
    tt = value >> (24 - (i << 3));
    for(j = 0; j < 2; j++)
      string[(i << 1) + j] = duotrice[(tt >> (4 - (j << 2))) & 0x0f];
  }
  return;
}

unsigned long updatecrc32(unsigned long crc, char c)
{
  return (cr3tab[((int)crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFF));
}

unsigned long crc32block(unsigned char *ptr, int count, unsigned long tcrc)
{
  int i;
  char *temp;

  temp = ptr;
  for(i = 0; i < count; i++, temp++)
  {
    tcrc = updatecrc32(tcrc, *temp);
  }
  return tcrc;
}

void converttime(char *time)
{
  // пpеобpазует стpоку time пpоизвольного фоpмата в стpоку по FTS-1
  unsigned int i;
  struct dostime ttmt;
  char *temp;

  temp = time;
  while(*temp)
  {
    while(isspace(*temp))
      temp++;
    if(isdigit(*temp))
    {
      i = atoi(temp);
      while(isdigit(*temp))
	temp++;
      if(isspace(*temp))
	if(i > 31)
	{
	  if(i >= 1900)
	    i -= 1900;
	  ttmt.year = i;
	}
	else
	  ttmt.day = i;
      else
      {
	ttmt.hour = i;
	while(!isdigit(*temp))
	  temp++;
	ttmt.min = atoi(temp);
	while(isdigit(*temp))
	  temp++;
	while(!isdigit(*temp))
	  temp++;
	ttmt.sec = atoi(temp);
      }
    }
    else
    {
      for(i = 0; i < 12; i++)
	if(memicmp(temp, months[i], 3) == 0)
	  break;
      if(i < 12)
	ttmt.month = i;
    }
    while(*temp && !isspace(*temp++)) ;
  }
  if(ttmt.day < 10)
    ftstime[0] = '0';
  else
    ftstime[0] = ttmt.day / 10 + '0';
  ftstime[1] = ttmt.day % 10 + '0';
  ftstime[2] = ' ';
  memcpy(ftstime + 3, months[ttmt.month], 3);
  ftstime[6] = ' ';
  if(ttmt.year < 10)
    ftstime[7] = '0';
  else
    ftstime[7] = ttmt.year / 10 + '0';
  ftstime[8] = ttmt.year % 10 + '0';
  ftstime[9] = ' ';
  ftstime[10] = ' ';
  if(ttmt.hour < 10)
    ftstime[11] = '0';
  else
    ftstime[11] = ttmt.hour / 10 + '0';
  ftstime[12] = ttmt.hour % 10 + '0';
  ftstime[13] = ':';
  if(ttmt.min < 10)
    ftstime[14] = '0';
  else
    ftstime[14] = ttmt.min / 10 + '0';
  ftstime[15] = ttmt.min % 10 + '0';
  ftstime[16] = ':';
  if(ttmt.sec < 10)
    ftstime[17] = '0';
  else
    ftstime[17] = ttmt.sec / 10 + '0';
  ftstime[18] = ttmt.sec % 10 + '0';
  ftstime[19] = 0;
}
