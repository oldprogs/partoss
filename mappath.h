#ifndef __MAPPATH_H_
#define __MAPPATH_H_
#include <stdlib.h>

#include "astring.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

struct mpath
{
  bool softremap;
};


class mappath
{
protected:
  int count;

public:
    stringlist dospath;
  stringlist lnxpath;
// struct mpath **path;

  int addpath (astring _src, astring _dest, bool _softremap);
  astring remap (char *_path);
    mappath ();
   ~mappath ();
};

extern mappath pmapper;

#endif
