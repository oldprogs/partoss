void addshort (struct shortchain *chain, unsigned short net, unsigned short node);
void delshort (struct shortchain *chain);
short addaddr (struct addrchain *chain, struct myaddr *addr);
void deladdr (struct addrchain *chain);
void addname (struct namechain *chain, struct sysname *name);
void delname (struct namechain *chain);
void addlink (struct linkchain *chain, struct link *link);
void dellink (struct linkchain *chain);
