void writechain (short handle, struct shortchain *chain, short type);
void readchain (short type, short subtype);
char *locseenby (char *buf);
char *locpath (char *buf);
void parsesnb (char *temp, short type);
void sortsnb (struct shortchain *chain);
void quicksort (unsigned long *array, unsigned long *array2);
void getctrl (char *text, unsigned short len, short type);
void delctrl (short type);
