// MaxiM: Ported.

#include "partoss.h"
#include "globext.h"

void readsetup(void)
{
  short i,
    j,
    fset,
    inset,
    outset,
    servset,
    wo;
  char *temp = NULL,
    firstnet[arealength],
    logpath[(DirSize + 1)];
  struct myaddr *taddr = NULL;
  struct uplname *tupl = NULL,
   *ttupl = NULL;
  struct link *tblink = NULL;
  struct pktname *tinbound = NULL;

  mystrncpy(cfname, mainconf, DirSize);
  temp = strrchr(cfname, '.');
  if(temp)
    *temp = 0;
  mystrncat(cfname, ".cfa", 6, DirSize);
  runmainset();
  gran = (short)(1 << bcfg.grof);
  // directvideo=bcfg.video;
  ndxlen = filelength(areandx);
  numarea = (short)(ndxlen / szareaindex);
  lseek(areaset, 0, SEEK_SET);
  lseek(areandx, 0, SEEK_SET);
  lseek(areadesc, 0, SEEK_SET);
  tlist = rlist;
  while(1)
  {
    tlist->alist =
      (struct areaindex *)myalloc((szareaindex << bcfg.grof), __FILE__,
                  __LINE__);
    tlist->next = NULL;
    if(rread
    (areandx, tlist->alist, (unsigned short)(szareaindex << bcfg.grof),
     __FILE__, __LINE__) < (szareaindex << bcfg.grof))
    {
      tlist->numlists = (short)(numarea & (gran - 1));
      break;
    }
    else
      tlist->numlists = gran;
    tlist->next =
      (struct alists *)myalloc(sizeof(struct alists), __FILE__, __LINE__);
    tlist = tlist->next;
  }
  fset = inset = outset = servset = 0;
  if(!bcfg.netin[0])
    inset = 1;
  if(!bcfg.netout[0])
    outset = 1;
  if(!bcfg.netserv[0])
    servset = 1;
  tlist = rlist;
  while(tlist)
  {
    for(i = 0; i < tlist->numlists; i++)
    {
      if(tlist->alist[i].type == 0)
      {
    lseek(areaset, tlist->alist[i].areaoffs, SEEK_SET);
    rread(areaset, newarea, szarea, __FILE__, __LINE__);
    if(!fset)
    {
      mystrncpy(inpath, newarea->areafp, DirSize);
      mystrncpy(outpath, newarea->areafp, DirSize);
      mystrncpy(firstnet, newarea->areaname, arealength - 1);
      fset = 1;
    }
    if(!inset)
    {
      if((strlen(bcfg.netin) == strlen(newarea->areaname))
          && (strnicmp(bcfg.netin, newarea->areaname, strlen(bcfg.netin))
          == 0))
      {
        mystrncpy(inpath, newarea->areafp, DirSize);
        inset = 1;
      }
    }
    if(!outset)
    {
      if((strlen(bcfg.netout) == strlen(newarea->areaname))
          &&
          (strnicmp(bcfg.netout, newarea->areaname, strlen(bcfg.netout))
           == 0))
      {
        mystrncpy(outpath, newarea->areafp, DirSize);
        outset = 1;
      }
    }
    if(!servset)
    {
      if((strlen(bcfg.netserv) == strlen(newarea->areaname))
          &&
          (strnicmp(bcfg.netserv, newarea->areaname, strlen(bcfg.netserv))
           == 0))
      {
        mystrncpy(servpath, newarea->areafp, DirSize);
        servset = 1;
      }
    }
    if(fset && inset && outset && servset)
      goto nfound;
      }
    }
    tlist = tlist->next;
  }
nfound:
  if(!bcfg.netin[0])
    mystrncpy(bcfg.netin, firstnet, arealength - 1);
  if(!bcfg.netout[0])
    mystrncpy(bcfg.netout, firstnet, arealength - 1);
  runcompset();
  if(bcfg.pack)
    runroute();
  taddr = bcfg.address.chain;
  tblink = bcfg.links.chain;
  for(i = 0; i < bcfg.numlink; i++)
  {
    if(tblink->packer[0] == 0)
      mystrncpy(tblink->packer, bcfg.defarc, 9);
    if(bcfg.passci)
      for(j = 0; j < 8; j++)
    tblink->password[j] = (char)toupper(tblink->password[j]);
    tblink = tblink->next;
  }
  bcfg.isnode = 0;
  while(taddr)
  {
    if(taddr->point == 0)
      bcfg.isnode = 1;
    taddr = taddr->next;
  }
#ifdef __DOS__
  if(bcfg.swapping)
  {
    createpath(bcfg.swapfile);
    mystrncat(bcfg.swapfile, "$ptswap$.$$$", 13, DirSize);
  }
#endif
  tinbound = bcfg.inbound;
  while(tinbound)
  {
    createpath(tinbound->name);
    tinbound = tinbound->next;
  }
  mystrncpy(outbound, bcfg.outbound, DirSize);
#ifdef __LNX__
  mystrncat(outbound, ".sq/", 6, DirSize);
#else
  mystrncat(outbound, ".sq\\", 6, DirSize);
#endif
  createpath(outbound);
  if(bcfg.workout[0] == 0)
    mystrncpy(bcfg.workout, outbound, DirSize);
  wo = (short)strlen(bcfg.workout);
  if(bcfg.workout[wo - 1] != DIRSEP[0])
  {
    bcfg.workout[wo] = DIRSEP[0];
    bcfg.workout[wo + 1] = 0;
  }
  createpath(bcfg.workout);
  createpath(bcfg.areapath);
  createpath(inpath);
  createpath(outpath);
  if(bcfg.netserv[0])
    createpath(servpath);
  createpath(bcfg.boxes);
  createpath(bcfg.busy);
  createpath(bcfg.workout);
  if(bcfg.workdir[0])
    createpath(bcfg.workdir);
  else
    mystrncpy(bcfg.workdir, homedir, DirSize);
  if(bcfg.logfile[0])
  {
#ifdef __LNX__
    temp = strrchr(bcfg.logfile, '/');
#else
    temp = strrchr(bcfg.logfile, '\\');
#endif
    memset(logpath, 0, DirSize);
    if(temp)
    {
      memcpy(logpath, bcfg.logfile, (unsigned)(temp - bcfg.logfile + 1));
      createpath(logpath);
    }
  }
  if(bcfg.semaphore)
  {
    tupl = bcfg.semaphore;
    while(tupl)
    {
      temp = strrchr(tupl->echolist, DIRSEP[0]);
      memset(logout, 0, BufSize + 1);
      if(temp)
      {
    memcpy(logout, tupl->echolist, (unsigned)(temp - tupl->echolist + 1));
    createpath(logout);
      }
      tupl = tupl->next;
    }
  }
}
