//MaxiM:  fully not ported, not checked

/***ash  Use wlink OPTION STACK instead _stklen
extern unsigned _stklen;
 */

extern char version[];
extern char *duotrice;
extern char *indicator;
extern short indpos;

extern int hhandle, hdst, holdh;
extern int hhandle2, hdst2, holdh2;

extern char *arcsh[7];
extern char confile[(DirSize + 1)];
extern char *keys[];
extern char *days[];
extern char *months[];
extern char *flags[];

extern unsigned short _openfd[MAX_HANDLES + 1];
extern unsigned char newHandleArray[MAX_HANDLES + 1];
extern unsigned char *oldArray;
extern unsigned short hcnt, oldcnt, afh, cfh, nfh;

extern char ppassword[9];

extern unsigned long firstkey, secondkey, firsttest, secondtest;
extern unsigned long namehash, namecrc, addrhash, addrcrc;
extern unsigned short ver, indv;

extern short isdupe, create, mustcopy, addorig, unclosed, lineno[4], noreport,
  forsed, fake, hide;
extern unsigned short dupecur, dupemax, local, netmail, flags2, lkludnum,
  lkludlen, gkludlen;
extern unsigned long msgcrc32, smsgcrc32, *dupbuf, mflags;
extern unsigned short szpacker, szmessage, szchar, szlong, sztwoplus,
  szpackmess, szpacket, szmyaddr, szincl;
extern unsigned short szbincfg, szshortaddr, szsysname, szzgate, szarea,
  szareaindex, szlink, szsqdfile;
extern unsigned short szpktname, szkludge, szroute;
extern unsigned short tottoss, loctoss, totsent, locsent, totdupes, totbad,
  totpers, numlf, rehwm;

extern short pkt, setf, numpack, dupreserv, logfile, tempsrv, thelp, crtrep,
  prttemp;
extern long pctrlpos, ptolen, pfromlen, psubjlen, ptextlen, pmsglen, pbigmess,
  pcmsglen;
extern long sctrlpos, stolen, sfromlen, ssubjlen, stextlen, smsglen, sbigmess,
  scmsglen;
extern long mctrlpos, mtolen, mfromlen, msubjlen, mtextlen, mmsglen, mbigmess,
  rmbigmess;
extern short mode, quiet, secure, statistic, ptnoscan, good, pgood, writes,
  wipearea, posting, diskfull, arealock;
extern short nummsg, endmsg, temppkt, tempsqd, tempmsg, badarea, mustlog,
  alog, blog, wassend, badtype;
extern short endinput[4], endblock[4], endstring[4], numcr[4], toklen,
  needout, fatmode;
extern short mainset, areaset, areandx, areadesc, areapool, arcset, compset,
  packset, linkset, numarea, pos, arealen, ttempl;
extern short arealenold;
extern short badlocked, dupelocked, mustdie, maxtaglen, globbad;
extern unsigned short fmax, buflen, pcurpos, scurpos, both, rescan, carea,
  linktype;
extern unsigned short pkludnum, skludnum, mkludnum, pkludlen, skludlen,
  mkludlen;
extern unsigned curspos[4], curtpos[4], maxpos, maxstr[4], maxstr2[4],
  maxlen[4];
extern long pointer, poffset, soffset, ndxlen, times, arclen, globold,
  globnew;
extern unsigned long *msgid, *reply;
extern unsigned long arctime, gstamp;
extern time_t sftime;
extern struct tm *tmt;

extern char filemsg[(DirSize + 1)], inpath[(DirSize + 1)],
  outpath[(DirSize + 1)], servpath[(DirSize + 1)], strtimes[9];
extern char *pktbuf, *sqdbuf, *sbuffer[4], *string, *token, *point,
  tstrtime[40];
extern char curarea[arealength], rcurarea[arealength], oldarea[arealength],
  tarea[arealength], defarc[10];
extern char areasbbs[(DirSize + 1)], mainconf[(DirSize + 1)],
  echolog[(DirSize + 1)], echologt[(DirSize + 1)], areafile[(DirSize + 1)],
  arealist[(DirSize + 1)], outbound[(DirSize + 1)], ftstime[20],
  cfname[DirSize + 1], blogname[DirSize + 1], homedir[DirSize + 1],
  logfilename[DirSize + 1];
extern short logfileok;
extern char logout[BufSize + 1], *tail, *tbuffer, temppktn[DirSize + 1],
  tempsqdn[DirSize + 1];
extern char gltoname[36], glfromname[36], glsubj[72], errname[DirSize + 1];
extern char curmsgid[9], curreply[9];
extern char bsyname[DirSize + 1], bsynamef[DirSize + 1],
  crtreprt[DirSize + 1];

extern struct packmess bufpkt, bufsqd, bufmess, *buftemp, *buftemps;
extern struct message bufmsg;
extern struct sqdfile sqbuf;
extern struct sqdmshead sqhead, head;
extern struct sqifile sqibuf;
//extern struct ftime ftm;
extern struct tm ftm;
extern struct myaddr pktaddr, node, mfnode, ufnode, rnode, defaddr, caddr;
extern struct myaddr *tsend;
extern struct addrchain ptosend, stosend;
extern struct area *newarea, *badmess, *dupes, *persarea;
extern struct bincfg bcfg;
extern struct packer pack;
extern struct packet *pkts, *tpack;
extern struct pktname *fpkt, *apkt;
extern struct shortchain psnb, ppath, ssnb, spath, tnsnb;
extern struct kludge *pckludge, *sckludge, *mckludge;
extern struct route *roulist, *direct, *notouch;
extern struct incl *tincl, *ttincl;

extern struct alists *rlist, *tlist, *ttlist;
extern unsigned short gran, bufsqi;
extern struct sqifile *nindex, *pindex, *bindex, *dindex;
extern short edlist;
extern short gheadclen;
extern short gheadgarb;
extern short addrfake;
extern short gpers;
extern short pcreate;
extern short glmove;
extern short rescandays;
extern short logdebug;
extern short lich;
extern short setdir;
extern short fullscan;
extern short findattr;
extern short nowpurge;
extern struct myaddr pfrom, pto;
extern unsigned long curhash;

extern struct pool *glpool;

extern list < areaalias > areaaliaslist;

extern long int umask_val;

extern unsigned short dnsipkt;
extern unsigned short dncloselog;

extern char pttmpl[DirSize + 1];
