void makeattach (short type);
void msgwrite (struct myaddr *from, struct myaddr *to, char *fromname,
               char *toname, char *subj, unsigned short flags, char *klflags,
               char *text);
void createpath (char *path);
void adjustname (char *name);
char *makebox (struct myaddr *toaddr, int type, int hold);
