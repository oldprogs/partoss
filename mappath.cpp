#ifndef __MAPPATH_CC_
#define __MAPPATH_CC_
#include "mappath.h"
#include <string.h>
#include <stdio.h>

mappath pmapper;

mappath::mappath ()
{
// path=(struct mpath **)malloc(sizeof(struct mpath *));
// if(path==NULL) perror("malloc");
  count = 0;
}

mappath::~mappath ()
{
//int i;
// if(count) for(i=0;i<count;i++) free(path[i]);
// free(path);
}

int mappath::addpath (astring _src, astring _dest, bool _softremap)
{
// path=(struct mpath **)realloc(path,sizeof(struct mpath *)*(count+1));
// if(path==NULL) {perror("malloc");return count;}
// path[count]=(struct mpath *)malloc(sizeof(struct mpath));
// if(path[count]==NULL) {count--;perror("malloc");return count;}
  dospath.add (_src);
  lnxpath.add (_dest);
// path[count]->softremap=_softremap;
  count++;
  return count - 1;
}

astring mappath::remap (char *_path)
{
  int i, l;
  astring temp_path = _path, t_path = _path;
  for (i = 0; i < count; i++)
    {
      if ((l = temp_path.icpos (dospath[i])))
	{
	  t_path = temp_path.substring (1, l - 1);
	  t_path += lnxpath[i];
	  t_path +=
	    temp_path.substring (l + dospath[i].len (),
				 temp_path.len () - l - dospath[i].len () +
				 1);
	  break;
	}
    }
  t_path = t_path.cvtslash ();
#ifdef __REMAP_LOWER__
  t_path = t_path.lower ();
#endif
//Here is very dangerous code!
  memset (_path, 0, t_path.len () + 1);
  strncpy (_path, t_path.c_str (), t_path.len ());
// printf("Remap: %s | %s\n",_path,temp_path.c_str());
  return t_path;
}
#endif
