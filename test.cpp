#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "_portlib.h"

char *rewrite(FILE * __handle)
{
  char *tmp_name;
  int ch;
  FILE *out;

  tmp_name = "/tmp/temp_file";
  if(tmp_name == NULL)
    return NULL;
  ccprintf("Temp: %s\n", tmp_name);
  out = fopen(tmp_name, "wb");
  while(!feof(__handle))
  {
    ch = fgetc(__handle);
    if(feof(__handle))
      break;
    if(ch != 13)
      fputc(ch, out);
  }
  fclose(out);
  return tmp_name;
}


void main(void)
{
  find_t ffblk;
  char *filename,
   *temp_file;
  int found;
  FILE *__handle;

  filename = (char *)calloc(256, 1);
  found = _dos_findfirst("/fido/sys/partoss/*", 0, &ffblk);
  while(!found)
  {
    ccprintf("Found: %s\n", ffblk.name);
    sprintf(filename, "/fido/sys/partoss/%s", ffblk.name);
    ccprintf("Filename: %s\n", filename);
    __handle = fopen(filename, "r+b");
    if(__handle == NULL)
    {
      perror("fopen");
      break;
    }
    temp_file = rewrite(__handle);
    fclose(__handle);
    if(temp_file != NULL)
    {
      rename(temp_file, filename);
    }
    found = _dos_findnext(&ffblk);
  }
  _dos_findclose(&ffblk);

}
