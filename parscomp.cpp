// MaxiM: memicmp

#include "partoss.h"
#include "globext.h"

#include "archives.h"

char *arckeys[] = {
  "Archiver",
  "Extension",
  "Ident",
  "Add",
  "Extract",
  "View",
  "End",

#if defined( __OS2__ )

  "OS2",
  "DOS",
  "W32",
  "LNX",
  "BSD"
#elif defined( __NT__ )

  "W32",
  "DOS",
  "OS2",
  "LNX",
  "BSD"
#elif defined(__linux__)

  "LNX",
  "BSD",
  "W32",
  "DOS",
  "OS2"
#elif defined (__FreeBSD__)

  "BSD",
  "LNX",
  "W32",
  "DOS",
  "OS2"
#else

  "DOS",
  "OS2",
  "W32",
  "LNX",
  "BSD"
#endif
};

void runcompset (void)
{
  short i, numarc = 12, finish, left = 0, radd, rextr;
  char *temp = NULL, hfile[DirSize + 1];
  cfname[strlen (cfname) - 1] = 'c';
  finish = 0;
  while (!finish)
    {
      compset = (short)sopen (cfname, O_RDWR | O_BINARY, SH_DENYNO);
      if (compset == -1)
  {
    switch (errno)
      {
      case ENOENT:
        if ((compset =
       (short)sopen (cfname, O_RDWR | O_BINARY | O_CREAT,
         SH_DENYWR,
         S_IRWXU | S_IRWXG | S_IRWXO)) == -1)
    {
      mystrncpy (errname, cfname, DirSize);
      errexit (2, __FILE__, __LINE__);
    }
        finish = 1;
        break;
      case EMFILE:
        mystrncpy (errname, cfname, DirSize);
        errexit (2, __FILE__, __LINE__);
      case EACCES:
        if (lich)
    {
      sprintf (logout, "Waiting for open/create %s", cfname);
      logwrite (1, 1);
    }
        mtsleep (5);
        break;
      default:
        mystrncpy (errname, cfname, DirSize);
        errexit (2, __FILE__, __LINE__);
      }
  }
      else
  finish = 1;
    }
  chsize (compset, 0);
  lseek (compset, 0, SEEK_SET);
  addhome (hfile, bcfg.packcfg);
  packset = mysopen (hfile, 0, __FILE__, __LINE__);
  mystrncpy (confile, hfile, DirSize);
  lineno[0] = 1;
  endinput[0] = 0;
  while (!endinput[0])
    {
      readblock (packset, 0);
      endblock[0] = 0;
      do
  {
    left = 0;
    getstring (0);
    gettoken (0);
    if (*token != ';')
      {
        for (i = 0; i < numarc; i++)
    if (memicmp (token, arckeys[i], strlen (arckeys[i])) == 0)
      break;
        if (i == 7)
    {
      left = 0;
      gettoken (0);
      for (i = 0; i < numarc; i++)
        if (memicmp (token, arckeys[i], strlen (arckeys[i])) == 0)
          break;
    }
        if (i == 8 || i == 9 || i == 10)
    {
      left = 1;
      gettoken (0);
      for (i = 0; i < numarc; i++)
        if (memicmp (token, arckeys[i], strlen (arckeys[i])) == 0)
          break;
    }
        switch (i)
    {
    default:
      tokencpy (logout, BufSize);
      if (strlen (logout))
        ccprintf ("Incorrect keyword \"%s\" in %s (line %d)\r\n",
            logout, bcfg.packcfg, lineno[0]);
      break;

    case 0:
      gettoken (0);
      memset (&pack, 0, szpacker);
      radd = rextr = 0;
      tokencpy (pack.name, 9);
      break;

    case 1:
      gettoken (0);
      tokencpy (pack.ext, 3);
      break;

    case 2:
      gettoken (0);
      pack.signoffs = atol (token);
      temp = strchr (token, ',');
      if (temp[1] == ' ')
        {
          gettoken (0);
          temp = token;
        }
      else
        temp++;
      if (left == 0 || pack.sign[0] == 0)
        {
          memcpy (pack.sign, temp,
            (unsigned)(toklen - (temp - token)));
          makebin (pack.sign);
        }
      break;

    case 3:
      gettoken (0);
      if (!left)
        {
          mystrncpy (pack.add, token,
         (short)(maxstr[0] - (token - ::string)));
          temp = strchr (token, '.');
          if (temp && ((temp - token) < toklen))
#if defined( __DOS__ )
      if (memicmp (temp, ".bat", 4) == 0)
#elif defined( __OS2__ )
      if (memicmp (temp, ".cmd", 4) == 0)
#else
      if ((memicmp (temp, ".cmd", 4) == 0)
          || (memicmp (temp, ".bat", 4) == 0))
#endif
        pack.addbat = 1;
          radd = 1;
        }
      else
        {
          if (!radd)
      {
        mystrncpy (pack.add, token,
             (short)(maxstr[0] - (token - ::string)));
        temp = strchr (token, '.');
        if (temp && ((temp - token) < toklen))
#if defined( __DOS__ )
          if (memicmp (temp, ".bat", 4) == 0)
#elif defined( __OS2__ )
          if (memicmp (temp, ".cmd", 4) == 0)
#else
          if ((memicmp (temp, ".cmd", 4) == 0)
        || (memicmp (temp, ".bat", 4) == 0))
#endif
            pack.addbat = 1;
      }
        }
      break;

    case 4:
      gettoken (0);
      if (!left)
        {
          mystrncpy (pack.extr, token,
         (short)(maxstr[0] - (token - ::string)));
          temp = strchr (token, '.');
          if (temp && ((temp - token) < toklen))
#if defined( __DOS__ )
      if (memicmp (temp, ".bat", 4) == 0)
#elif defined( __OS2__ )
      if (memicmp (temp, ".cmd", 4) == 0)
#else
      if ((memicmp (temp, ".cmd", 4) == 0)
          || (memicmp (temp, ".bat", 4) == 0))
#endif
        pack.extrbat = 1;
          rextr = 1;
        }
      else
        {
          if (!rextr)
      {
        mystrncpy (pack.extr, token,
             (short)(maxstr[0] - (token - ::string)));
        temp = strchr (token, '.');
        if (temp && ((temp - token) < toklen))
#if defined( __DOS__ )
          if (memicmp (temp, ".bat", 4) == 0)
#elif defined( __OS2__ )
          if (memicmp (temp, ".cmd", 4) == 0)
#else
          if ((memicmp (temp, ".cmd", 4) == 0)
        || (memicmp (temp, ".bat", 4) == 0))
#endif
            pack.extrbat = 1;
      }
        }
      break;

    case 5:
      break;

    case 6:
      lseek (compset, 0, SEEK_END);
      wwrite (compset, &pack, szpacker, __FILE__, __LINE__);
      break;

    case 8:
    case 9:
      break;
    }
      }
    lineno[0] += numcr[0];
  }
      while (!endblock[0]);
    }
  cclose (&packset, __FILE__, __LINE__);
//  cclose(&compset,__FILE__,__LINE__);
}
