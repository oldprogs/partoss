short setarea (char *areaname, short pers);
void setbadarea (void);
int openarea (unsigned short i, short pers);
void opensqd (struct area *ttarea, struct sqifile *tindex, short dup, short pers);
void closesqd (struct area *ttarea, short dup);
void createarea (char *areaname, short pers, struct myaddr *pktaddr2);
void writearea (short handle, struct area *ttarea, char *descr, short type);
