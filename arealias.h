
class areaalias
{
public:
  char sarea[arealength];
  char darea[arealength];
  struct myaddr linkaddr;

  int operator < (areaalias & a)
  {
    return (stricmp (sarea, a.sarea) < 0);
  }
  int operator > (areaalias & a)
  {
    return (stricmp (sarea, a.sarea) > 0);
  }
  int operator == (areaalias & a) {
    if (stricmp (sarea, a.sarea) != 0 || stricmp (darea, a.darea) != 0)
      return 0;
    if (linkaddr.zone != a.linkaddr.zone || linkaddr.net != a.linkaddr.net ||
  linkaddr.node != a.linkaddr.node
  || linkaddr.point != a.linkaddr.point)
      return 0;
    return 1;
  }
  areaalias & operator = (areaalias & a)
  {
    strcpy (sarea, a.sarea);
    strcpy (darea, a.darea);
    linkaddr.zone = a.linkaddr.zone;
    linkaddr.net = a.linkaddr.net;
    linkaddr.node = a.linkaddr.node;
    linkaddr.point = a.linkaddr.point;
    return *this;
  }
};

void areaaliasmaker ();
char *areaaliasrestorer (char *);

extern list < areaalias > areaaliaslist;
