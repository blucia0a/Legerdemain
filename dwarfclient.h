#define false 0
#define true 1

#include <libdwarf.h>

Dwarf_Debug d;
Dwarf_Error e;
unsigned long get_iaddr_of_file_line(const char *file, unsigned line);
void decode_location(Dwarf_Locdesc *locationList, Dwarf_Signed listLength, long *offset, long *init_val, int *frameRel);
void show_all_attrs(Dwarf_Die die,unsigned long level, void *d);
void visit_die(Dwarf_Die die, unsigned long level, void (*action)(Dwarf_Die,unsigned long, void*),void*adata);
void show_scopes_by_file_line(char *fileline_fn, int fileline_ln);
void show_scopes_by_addr(void *addr);
#ifdef DWARF_CLIENT_LIB
int getdwarfdata(char *argv);
#else
int main(int argc, char **argv);
#endif
