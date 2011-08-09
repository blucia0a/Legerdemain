/* addr2line.c -- convert addresses to line number and function name
 *    Copyright 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006, 2007
 *       Free Software Foundation, Inc.
 *          Contributed by Ulrich Lauther <Ulrich.Lauther@mchp.siemens.de>
 *
 *             This file is part of GNU Binutils.
 *
 *                This program is free software; you can redistribute it and/or modify
 *                   it under the terms of the GNU General Public License as published by
 *                      the Free Software Foundation; either version 3, or (at your option)
 *                         any later version.
 *
 *                            This program is distributed in the hope that it will be useful,
 *                               but WITHOUT ANY WARRANTY; without even the implied warranty of
 *                                  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *                                     GNU General Public License for more details.
 *
 *                                        You should have received a copy of the GNU General Public License
 *                                           along with this program; if not, write to the Free Software
 *                                              Foundation, 51 Franklin Street - Fifth Floor, Boston,
 *                                                 MA 02110-1301, USA.  */


/* Derived from objdump.c and nm.c by Ulrich.Lauther@mchp.siemens.de
 */ 

/* Modifications by Brandon Lucia, University of Washington -- 2011
 * libaddr2line - this is a hackjob re-write of addr2line so that I can call it directly in my
 *               code, rather than using a pipe.
 * include addr2line.h, and call get_info(char *<executable name>, unsigned long <instr addr>) 
 *               
 **/

#include "bfd.h"
#include "getopt.h"
#include "libiberty.h"
#include "stdlib.h"
#include "malloc.h"
#include "string.h"

static bfd_boolean unwind_inlines;	/* -i, unwind inlined functions. */
static bfd_boolean with_functions;	/* -f, show function names.  */
static bfd_boolean base_names;		/* -s, strip directory names.  */

static int naddr;		/* Number of addresses to process.  */
static char **addr;		/* Hex addresses to process.  */

static asymbol **syms;		/* Symbol table.  */

static void slurp_symtab (bfd *);
static void find_address_in_section (bfd *, asection *, void *);
static void find_offset_in_section (bfd *, asection *);
char *translate_address (unsigned long, bfd *, asection *);

/* Read in the symbol table.  */
  static void
slurp_symtab (bfd *abfd)
{
  long symcount;
  unsigned int size;

  if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
    return;

  symcount = bfd_read_minisymbols (abfd, FALSE, (void *) &syms, &size);
  if (symcount == 0)
    symcount = bfd_read_minisymbols (abfd, TRUE /* dynamic */, (void *) &syms, &size);

  if (symcount < 0)
    fprintf(stderr,"[A2L] Error slurping symtab for %s!\n",bfd_get_filename(abfd) );
    //bfd_fatal (bfd_get_filename (abfd));
}

/* These global variables are used to pass information between
 *    translate_addresses and find_address_in_section.  */
bfd_vma pc;
const char *filename;
const char *functionname;
unsigned int line;
bfd_boolean found;

/* Look for an address in a section.  This is called via
 *    bfd_map_over_sections.  */
  static void
find_address_in_section (bfd *abfd, asection *section,
    void *data ATTRIBUTE_UNUSED)
{
  bfd_vma vma;
  bfd_size_type size;

  if (found)
    return;

  if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
    return;

  vma = bfd_get_section_vma (abfd, section);
  if (pc < vma)
    return;

  size = bfd_get_section_size (section);
  if (pc >= vma + size)
    return;

  found = bfd_find_nearest_line (abfd, section, syms, pc - vma,
      &filename, &functionname, &line);
}

/* Look for an offset in a section.  This is directly called.  */
  static void
find_offset_in_section (bfd *abfd, asection *section)
{
  bfd_size_type size;

  if (found)
    return;

  if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
    return;

  size = bfd_get_section_size (section);
  if (pc >= size)
    return;

  found = bfd_find_nearest_line (abfd, section, syms, pc,
      &filename, &functionname, &line);
}

/* Read hexadecimal addresses from stdin, translate into
 *    file_name:line_number and optionally function name.  */
char *translate_address(unsigned long addr, bfd *abfd, asection *section) {

  char *output = NULL;
  pc = addr;//bfd_scan_vma((void*)addr, NULL, 16);
  found = 0;
  if (section){
    find_offset_in_section (abfd, section);
  }else{

    bfd_map_over_sections (abfd, find_address_in_section, NULL);

  }

  if (found){
    do {

      const char *name;
      char *alloc = NULL;
      name = functionname;
      if (name == NULL || *name == '\0'){
	name = "??";
      }

      if (base_names && filename != NULL)
      {
	char *h;
	h = strrchr (filename, '/');
	if (h != NULL)
	  filename = h + 1;
      }

     
      int numdig = 0;  
      int tline = line;
      while(tline > 0){
        numdig++;
        tline /= 10;
      }

      if( output == NULL ){
   
        int len = ( (filename ? strlen(filename) : 2) + 
                    (functionname ? strlen(functionname) : 2) + 
                    numdig + 
                    strlen("\n: ()") + 
                    2);
        output = (char *)malloc( len * sizeof(char));
        memset(output, 0, len * sizeof(char));
        sprintf(output,"%s:%d (%s)",
                filename ? filename : "??", 
                line, 
                functionname && strncmp(functionname,"??",2) ? functionname : "??");

      }else{

        char *newbuf;
        int len = ( (filename ? strlen(filename) : 2) + 
                    (functionname ? strlen(functionname) : 2) + 
                    numdig + 
                    strlen("\n: ()") + 
                    2);
        newbuf = (char *)malloc( len * sizeof(char));
        memset(output, 0, len * sizeof(char));
        sprintf(newbuf,"%s\n%s:%d (%s)",
                output, 
                filename ? filename : "??", 
                line, 
                functionname && strncmp(functionname,"??",2) ? functionname : "??");
      }

      found = bfd_find_inliner_info (abfd, &filename, &functionname, &line);

    }while (found);

  }
  return output == NULL ? strdup("<code unavailable>") : output;
}
/* Process a file.  Returns an exit value for main().  */
char *process_file (unsigned long addr, const char *file_name, const char *section_name, const char *target) {

  bfd *abfd;
  asection *section;
  char **matching;

  /*
 *  if (get_file_size (file_name) < 1)
    return 1;
  */

  abfd = bfd_openr (file_name, "default");
  if (abfd == NULL){
    fprintf(stderr, "[A2L] couldn't get source info for %s\n",file_name);
  }

  if (bfd_check_format (abfd, bfd_archive)){
    fprintf(stderr, "[A2L] couldn't get source info for %s\n",file_name);
  }
    //fatal (_("%s: cannot get addresses from archive"), file_name);

  if (! bfd_check_format_matches (abfd, bfd_object, &matching))
  {
    fprintf(stderr, "[A2L] couldn't get source info for %s\n",file_name);
    /*
    bfd_nonfatal (bfd_get_filename (abfd));
    if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
    {
      list_matching_formats (matching);
      free (matching);
    }
    xexit (1); 
    */
  }

  if (section_name != NULL)
  {
    section = bfd_get_section_by_name (abfd, section_name);
    if (section == NULL){
      fprintf(stderr, "[A2L] couldn't get section info for %s->%s\n",file_name,section_name);
      return strdup("<code unavailable>");
      //fatal (_("%s: cannot find section %s"), file_name, section_name);
    }
  }
  else
    section = NULL;

  slurp_symtab (abfd);

  char *ret = translate_address (addr, abfd, section);

  if (syms != NULL)
  {
    free (syms);
    syms = NULL;
  }

  bfd_close (abfd);

  return ret;
}
  
char *get_info(unsigned long addr, char *executable) {

  const char *file_name;
  const char *section_name;
  char *target;
  int c;

  bfd_init ();
  //set_default_bfd_target ();

  section_name = NULL;
  target = NULL;

  if (executable == NULL)
    return strdup("[A2L] No Executable Specified!");

  char * ret = process_file (addr, executable, section_name, target);
  return ret == NULL ? strdup("<code unavailable>") : ret;
  

}
