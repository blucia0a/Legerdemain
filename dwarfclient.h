#define false 0
#define true 1

#include <libdwarf.h>

Dwarf_Debug d;
Dwarf_Error e;
unsigned long get_iaddr_of_file_line(const char *file, unsigned line);
void show_scopes_by_file_line(char *fileline_fn, int fileline_ln);
void show_scopes_by_addr(void *addr);
void show_info_for_scoped_variable(void *addr, const char *varname);


#ifdef DWARF_CLIENT_LIB
int getdwarfdata(char *argv);
#else
int main(int argc, char **argv);
#endif
