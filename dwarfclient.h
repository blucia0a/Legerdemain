#define false 0
#define true 1

#include <libdwarf.h>

typedef struct _DC_location{

  int isFrameOffset;
  long offset;

} DC_location;

typedef struct _DC_type{

  unsigned indirectionLevel;
  char *name;
  unsigned long byteSize; 

} DC_type;

Dwarf_Debug d;
Dwarf_Error e;
unsigned long get_iaddr_of_file_line(const char *file, unsigned line);
void show_scopes_by_file_line(char *fileline_fn, int fileline_ln);
void show_scopes_by_addr(void *addr);
void show_info_for_scoped_variable(void *addr, const char *varname);
long get_location_of_scoped_variable(void *addr, const char *varname);
DC_type *get_type_of_scoped_variable(void *addr, const char *varname);
void show_vars_by_scope_addr(void *addr);

#ifdef DWARF_CLIENT_LIB
int getdwarfdata(char *argv);
#else
int main(int argc, char **argv);
#endif
