void delorphn (struct uplname *utarea, struct uplname *ttname, char * descr);
void delorph (struct uplname *utarea, struct uplname *ttname);
short delkill (short deltype);
void dolist (short rt);
void inecholog (char *areaname);
void addsarea (struct uplname **chain, struct uplname *tempor, char *areaname,
         short where, short wild);
void chareacfg (struct uplname *areas, char *file);
void chlinkcfg (struct link *blink, char *file);
void areasort (short sort);
