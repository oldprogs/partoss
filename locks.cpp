
#include "partoss.h"
#include "globext.h"

#include "locks.h"

int readlock (char * filename)
{
   int h, pid;
   char apid[20]; // ascii lock
   int length;

   if ((h = open(filename, O_RDONLY)) == -1)
      return -1; // open error

   length = read(h, apid, sizeof(apid) - 1);
   apid[length] = 0;

   pid = 0;
   if ((length == sizeof(pid)) || (sscanf(apid, "%d", &pid) != 1) || (pid == 0))
      pid = *((int *)apid); // ? binary lock

   close(h);
   return pid; // success
};

int writelock (char * filename)
{
   int h;
   char apid[16];

   if ((h = open(filename, O_CREAT|O_WRONLY|O_EXCL, 0644)) == -1)
      return -1; // create error

   sprintf(apid, "%10d\n", getpid());
   if (write(h, apid, strlen(apid)) != strlen(apid))
   {
      close(h);
      return -1; // write error
   };

   close(h);
   return 0; // success
};

int checklock (char * filename)
{
   int pid;
   struct stat st;
   
   if ((stat(filename, &st) == -1) && errno == ENOENT)
      return 0; // no file
   
   if ((pid = readlock(filename)) == -1)
      return 0; // read error

   if (pid == getpid())
      return 0; // ! our lock

   if ((kill(pid, 0) == -1) && errno == ESRCH)
   {
      unlink(filename);
      return 0; // no owner
   };
   
   return pid; // success
};

int removelock (char * filename)
{
   if (lck)
		if (unlink(filename) != -1)
         return 0; // success

   return -1; // error
};
