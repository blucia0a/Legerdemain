#define false 0
#define true 1

#include <libdwarf.h>

Dwarf_Debug d;
Dwarf_Error e;
void decode_location(Dwarf_Locdesc *locationList, Dwarf_Signed listLength, long *offset, long *init_val, int *frameRel);
void show_all_attrs(Dwarf_Die die);
void visit_die(Dwarf_Die die, unsigned int level);
#ifdef DWARF_CLIENT_LIB
int getdwarfdata(char *argv);
#else
int main(int argc, char **argv);
#endif
