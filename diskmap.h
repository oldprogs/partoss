#ifndef __DISKMAP__
#define  __DISKMAP__
#define MAXDRIVES ('Z'-'A')
class TDiskMap
{
private:
  char *map[MAXDRIVES];
public:
    TDiskMap ();
   ~TDiskMap ()
  {
    Remove ();
  }
  void Remove ();
  void Set (char disk, char *path);
  void Get (char disk, char *path);

  char *operator () (char *src)
  {
    return GetPath (src);
  };
  char *GetPath (char *dest, char *src);
  char *GetPath (char *src)
  {
    static char dest[256];
    return GetPath (dest, src);
  };

};
void set_dmap (TDiskMap * p, char *value);
#endif
