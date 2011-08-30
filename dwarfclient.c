#include <libdwarf.h>
#include <dwarf.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "stack.h"
#include "dwarfclient.h"

static void decode_location(Dwarf_Locdesc *locationList, Dwarf_Signed listLength, long *offset, long *init_val, int *frameRel){

  /*Location Decoding Code from http://ns.dyninst.org/coverage/dyninstAPI/src/parseDwarf.C.gcov.html*/
  Stack *opStack = (Stack*)malloc(sizeof(Stack));
  Stack_Init(opStack);
  assert( listLength > 0 );
 
  if( listLength > 1 ) { fprintf( stderr, "Warning: more than one location, ignoring all but first.\n" ); }
 
  /* Initialize the stack. */
  if( init_val != NULL ) { Stack_Push(opStack, * init_val); }
 
  /* Variable can only become frame-relative by using the frame pointer operand. */
  if( frameRel != NULL ) { * frameRel = false; }
 
  /* Ignore all but the first location, for now. */
  Dwarf_Locdesc location = locationList[0];
  Dwarf_Loc * locations = location.ld_s;
  unsigned int i;
  for( i = 0; i < location.ld_cents; i++ ) {
    /* Handle the literals w/o 32 case statements. */
    if( DW_OP_lit0 <= locations[i].lr_atom && locations[i].lr_atom <= DW_OP_lit31 ) {
      //fprintf( stderr, "Pushing named constant: %d\n", locations[i].lr_atom - DW_OP_lit0 );
      Stack_Push(opStack, locations[i].lr_atom - DW_OP_lit0 );
      continue;
    }
 
    if( (DW_OP_breg0 <= locations[i].lr_atom && locations[i].lr_atom <= DW_OP_breg31) || locations[i].lr_atom == DW_OP_bregx ) {
      /* Offsets from the specified register.  Since we're not
         doing this at runtime, we can't do anything useful here. */
      fprintf( stderr, "Warning: location decode requires run-time information, giving up.\n" );
      *offset = -1;
      return;
    }
 
    if( (DW_OP_reg0 <= locations[i].lr_atom && locations[i].lr_atom <= DW_OP_reg31) || locations[i].lr_atom == DW_OP_regx ) {
      /* The variable resides in the particular register.  We can't do anything
         useful with this information yet. */
      fprintf( stderr, "Warning: location decode indicates register, giving up.\n" );
      *offset = -1;
      return;
    }
 
    switch( locations[i].lr_atom ) {
      case DW_OP_addr:
      case DW_OP_const1u:
      case DW_OP_const2u:
      case DW_OP_const4u:
      case DW_OP_const8u:
      case DW_OP_constu:
        // fprintf( stderr, "Pushing constant %lu\n", (unsigned long)locations[i].lr_number );
        Stack_Push(opStack, (Dwarf_Unsigned)locations[i].lr_number );
        break;
 
      case DW_OP_const1s:
      case DW_OP_const2s:
      case DW_OP_const4s:
      case DW_OP_const8s:
      case DW_OP_consts:
        // fprintf( stderr, "Pushing constant %ld\n", (signed long)(locations[i].lr_number) );
        Stack_Push(opStack, (Dwarf_Signed)(locations[i].lr_number) );
        break;
 
      case DW_OP_fbreg:
        /* We're only interested in offsets, so don't add the FP. */
        // fprintf( stderr, "Pushing FP offset %ld\n", (signed long)(locations[i].lr_number) );
        Stack_Push(opStack, (Dwarf_Signed)(locations[i].lr_number) );
        if( frameRel != NULL ) { * frameRel = true; }
        break;
 
      case DW_OP_dup:
        Stack_Push(opStack, Stack_Top(opStack) );
        break;
 
      case DW_OP_drop:
        Stack_Pop(opStack);
        break;
 
      case DW_OP_over: {
          long int first = Stack_Top(opStack); Stack_Pop(opStack);
          long int second = Stack_Top(opStack); Stack_Pop(opStack);
          Stack_Push(opStack, second ); Stack_Push(opStack, first ); Stack_Push(opStack, second );
        } break;
 
      case DW_OP_pick: {
        /* Duplicate the entry at index locations[i].lr_number. */
        Stack temp;
        Stack_Init(&temp);
        unsigned int i;
        for( i = 0; i < locations[i].lr_number; i++ ) {
          Stack_Push(&temp, Stack_Top(opStack) ); Stack_Pop(opStack);
        }
        long int dup = Stack_Top(opStack);
        for( i = 0; i < locations[i].lr_number; i++ ) {
          Stack_Push(opStack, Stack_Top(&temp) ); Stack_Pop(&temp);
        }
        Stack_Push(opStack, dup );
        } break;
 
      case DW_OP_swap: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first ); Stack_Push(opStack, second );
      } break;
 
      case DW_OP_rot: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        long int third = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first ); Stack_Push(opStack, third ); Stack_Push(opStack, second );
      } break;
 
      case DW_OP_deref:
      case DW_OP_deref_size:
      case DW_OP_xderef:
      case DW_OP_xderef_size:
        // fprintf( stderr, "Warning: location decode requires run-time information, giving up.\n" );
        * offset = -1;
        return;
 
      case DW_OP_abs: {
        long int top = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, abs( top ) );
        } break;
 
      case DW_OP_and: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second & first );
      } break;
                  
      case DW_OP_div: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second / first );
      } break;
 
      case DW_OP_minus: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second - first );
      } break;
 
      case DW_OP_mod: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second % first );
      } break;
 
      case DW_OP_mul: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second * first );
      } break;
 
      case DW_OP_neg: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first * (-1) );
      } break;
 
      case DW_OP_not: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, ~ first );
      } break;
 
      case DW_OP_or: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second | first );
      } break;
 
      case DW_OP_plus: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second + first );
      } break;
 
      case DW_OP_plus_uconst: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first + locations[i].lr_number );
      } break;
 
      case DW_OP_shl: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second << first );
      } break;
 
      case DW_OP_shr: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, (long int)((unsigned long)second >> (unsigned long)first) );
      } break;
 
      case DW_OP_shra: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second >> first );
      } break;
 
      case DW_OP_xor: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, second ^ first );
      } break;
 
      case DW_OP_le: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first <= second ? 1 : 0 );
      } break;
 
      case DW_OP_ge: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first >= second ? 1 : 0 );
      } break;
 
      case DW_OP_eq: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first == second ? 1 : 0 );
      } break;
 
      case DW_OP_lt: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first < second ? 1 : 0 );
      } break;
 
      case DW_OP_gt: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first > second ? 1 : 0 );
      } break;
 
      case DW_OP_ne: {
        long int first = Stack_Top(opStack); Stack_Pop(opStack);
        long int second = Stack_Top(opStack); Stack_Pop(opStack);
        Stack_Push(opStack, first != second ? 1 : 0 );
      } break;
 
      case DW_OP_bra:
        if( Stack_Top(opStack) == 0 ) { break; }
        Stack_Pop(opStack);
        break;/*Right?*/
      case DW_OP_skip: {
        int bytes = (int)(Dwarf_Signed)locations[i].lr_number;
        unsigned int target = locations[i].lr_offset + bytes;
 
        int j = i;
        if( bytes < 0 ) {
          for( j = i - 1; j >= 0; j-- ) {
            if( locations[j].lr_offset == target ) { break; }
          } /* end search backward */
        } else {
          for( j = i + 1; j < location.ld_cents; j ++ ) {
            if( locations[j].lr_offset == target ) { break; }
          } /* end search forward */
        } /* end if positive offset */
 
        /* Because i will be incremented the next time around the loop. */
        i = j - 1;
      } break;
 
      case DW_OP_piece:
        /* For multi-part variables, which we don't handle. */
        break;
 
      case DW_OP_nop:
        break;
 
      default:
        fprintf( stderr, "Unrecognized location opcode 0x%x, returning offset -1.\n", locations[i].lr_atom );
        *offset = -1;
        return;
    } /* end operand switch */
  } /* end iteration over Dwarf_Loc entries. */
 
  /* The top of the stack is the computed location. */
  //fprintf( stderr, "Location decoded: %ld\n", Stack_Top(opStack) );
  *offset = Stack_Top(opStack);
}  

static void DC_get_location_attr_value(Dwarf_Attribute at, DC_location *dcl){
  /*
   * It is assumed that at has a block form, and is a location list
   * so this won't fail.  Caller is responsible for checking.
   */

  Dwarf_Locdesc *locationList;
  Dwarf_Signed listLength;
  Dwarf_Error error;
  int frameRel = 0;
  long offset = 0;
  int i;
  int ret = dwarf_loclist( at, &locationList, &listLength, &error );
  if( ret != DW_DLV_OK ){
    return;
  }

  /*Get the location info*/
  decode_location(locationList,listLength,&(dcl->offset),NULL,&(dcl->isFrameOffset));

  /*Clean up*/
  for( i = 0; i < listLength; ++i){
    dwarf_dealloc(d,locationList[i].ld_s,DW_DLA_LOC_BLOCK);               
  }
  dwarf_dealloc(d,locationList,DW_DLA_LOCDESC);               
  
}


static void show_all_attrs(Dwarf_Die die, unsigned long level, void *ndata){
  Dwarf_Error error;
  Dwarf_Half tag; 
  dwarf_tag(die,&tag,&error);

  const char *stag;
  dwarf_get_TAG_name(tag,&stag); 

  Dwarf_Off off = 0x0;
  dwarf_die_CU_offset(die,&off,&error);
  fprintf(stderr,"[%u]<%x>%s\n",level,off,stag);
  
  char **sourceFiles;
  Dwarf_Signed num;
  int res;
  if( (res = dwarf_srcfiles(die,&sourceFiles,&num,&error)) == DW_DLV_OK){
    fprintf(stderr,"Source Files Referenced:\n");
    int i;
    for(i = 0; i < num; i++){
      fprintf(stderr,"%s\n",sourceFiles[i]);
      dwarf_dealloc(d, sourceFiles[i],DW_DLA_STRING);
    } 
    dwarf_dealloc(d, sourceFiles,DW_DLA_LIST);
  }

  Dwarf_Unsigned atcnt;
  Dwarf_Attribute *atlist;
  int errv;
  if((errv = dwarf_attrlist(die, &atlist, &atcnt, &error)) == DW_DLV_OK){
    int i;
    for(i = 0; i < atcnt; i++){
      Dwarf_Half attr;
      if(dwarf_whatattr(atlist[i],&attr,&error) == DW_DLV_OK){
        const char *sattr;
        dwarf_get_AT_name(attr,&sattr); 
        fprintf(stderr,"\t%s => ",sattr);
      }else{
        fprintf(stderr,"\tCouldn't Get Attr Type!\n"); 
        continue;
      }
      Dwarf_Half form;
      if(dwarf_whatform(atlist[i],&form,&error) == DW_DLV_OK){
        //const char *formname;
        //dwarf_get_FORM_name(form,&formname);
        //fprintf(stderr,"[%s] ",formname);
        switch(form){
          case DW_FORM_ref1:
          case DW_FORM_ref2:
          case DW_FORM_ref4:
          case DW_FORM_ref8:
          case DW_FORM_ref_udata:
            {Dwarf_Off offset;
            dwarf_formref(atlist[i],&offset,&error); 
            fprintf(stderr,"%x\n",offset);}
            break;
          case DW_FORM_ref_addr:
            {Dwarf_Off offset;
            dwarf_global_formref(atlist[i],&offset,&error); 
            fprintf(stderr,"%x\n",offset);}
            break;
          case DW_FORM_addr:
            {Dwarf_Addr addr;
            dwarf_formaddr(atlist[i],&addr,&error); 
            fprintf(stderr,"%x\n",addr);}
            break;
          case DW_FORM_flag:
            {Dwarf_Bool flag;
            dwarf_formflag(atlist[i],&flag,&error); 
            fprintf(stderr,"%s\n",flag ? "True" : "False");}
            break;
          case DW_FORM_udata:
          case DW_FORM_data1:
          case DW_FORM_data2:
          case DW_FORM_data4:
          case DW_FORM_data8:
            {Dwarf_Unsigned val;
            dwarf_formudata(atlist[i],&val,&error); 
            fprintf(stderr,"%u\n",val);}
            break;
          case DW_FORM_sdata:
            {Dwarf_Signed val;
            dwarf_formsdata(atlist[i],&val,&error); 
            fprintf(stderr,"%d\n",val);}
            break;
          case DW_FORM_block:
          case DW_FORM_block1:
            if(attr == DW_AT_location ||
               attr == DW_AT_data_member_location ||
               attr == DW_AT_vtable_elem_location ||
               attr == DW_AT_string_length ||
               attr == DW_AT_use_location ||
               attr == DW_AT_return_addr){
              /* 
              Dwarf_Locdesc *locationList;
              Dwarf_Signed listLength;
              int ret = dwarf_loclist( atlist[i], &locationList, &listLength, &error );
              int frameRel = 0;
              long offset = 0;
              decode_location(locationList,listLength,&offset,NULL,&frameRel);
              int i;
              for( i = 0; i < listLength; ++i){
                dwarf_dealloc(d,locationList[i].ld_s,DW_DLA_LOC_BLOCK);               
              }
              dwarf_dealloc(d,locationList,DW_DLA_LOCDESC);               
              */
              DC_location dcl;
              DC_get_location_attr_value(atlist[i],&dcl); 
              fprintf(stderr," %s:",dcl.isFrameOffset ? "FP Offset" : "Address");
              fprintf(stderr," %ld\n",dcl.offset);

            }else{
              fprintf(stderr,"UNSUPPORTED ATTRIBUTE TYPE\n");
            }
            break;
          case DW_FORM_string:
            {char *val;
            dwarf_formstring(atlist[i],&val,&error); 
            fprintf(stderr,"%s\n",val);}
            break;
          
          case DW_FORM_strp:
            {char *str;
            if( (dwarf_formstring(atlist[i],&str,&error) == DW_DLV_OK) ){
              fprintf(stderr,"%s\n",str);
            } }
            break;
          
          default:
            fprintf(stderr,"Unhandled Attribute Form!\n");
            break;
            
        };
      }
      dwarf_dealloc(d, atlist[i], DW_DLA_ATTR);
    }
    dwarf_dealloc(d, atlist, DW_DLA_LIST);
  } 

}

static Dwarf_Die get_cu_by_iaddr(unsigned long iaddr){
 
  Dwarf_Unsigned cu_h_len;
  Dwarf_Half verstamp;
  Dwarf_Unsigned abbrev_offset;
  Dwarf_Half addrsize;
  Dwarf_Unsigned next_cu;
  Dwarf_Error error; 
  while( dwarf_next_cu_header(d,
                              &cu_h_len,
                              &verstamp,
                              &abbrev_offset,
                              &addrsize,
                              &next_cu,
                              &error) == DW_DLV_OK ){

    Dwarf_Die cu_die = NULL;
    int sibret;
 
    int dieno = 0; 
    while((sibret = 
           dwarf_siblingof(d,cu_die,&cu_die,&error)) != DW_DLV_NO_ENTRY &&
           sibret                                    != DW_DLV_ERROR){

      Dwarf_Attribute lowattr;
      if( dwarf_attr(cu_die, DW_AT_low_pc, &lowattr, &error) != DW_DLV_OK ){
        continue;
      }
      Dwarf_Attribute highattr;
      if( dwarf_attr(cu_die, DW_AT_high_pc, &highattr, &error) != DW_DLV_OK ){
        continue;
      }

      Dwarf_Addr loval,hival;
      dwarf_formaddr(lowattr,&loval,&error); 
      dwarf_formaddr(highattr,&hival,&error); 
      if(iaddr >= loval && iaddr <= hival){
        return cu_die;
      }

    }

  }
  return (Dwarf_Die)-1;
  
}
 

static void visit_die(Dwarf_Die die, unsigned long level, void (*action)(Dwarf_Die,unsigned long, void *d),void *adata){

  /*This function visits the children of a die in sequence, 
   *applying the action() function to each*/
  action(die,level,adata);
 
  Dwarf_Error error; 
  Dwarf_Die kid;
  if( dwarf_child(die,&kid,&error) == DW_DLV_NO_ENTRY ){
    return;
  }
  visit_die(kid,level+1,action,adata); 

  int chret;
  while( (chret = dwarf_siblingof(d,kid,&kid,&error)) != DW_DLV_NO_ENTRY &&
           chret != DW_DLV_ERROR){

    visit_die(kid,level+1,action,adata);

  }

  return;
  
}

static void reset_cu_header_info(){
  Dwarf_Unsigned cu_h_len;
  Dwarf_Half verstamp;
  Dwarf_Unsigned abbrev_offset;
  Dwarf_Half addrsize;
  Dwarf_Unsigned next_cu;
  Dwarf_Error error; 
  while( dwarf_next_cu_header(d,&cu_h_len,&verstamp,&abbrev_offset,&addrsize,&next_cu,&error) != DW_DLV_NO_ENTRY );
}

static void DC_get_info_for_scoped_variable(Dwarf_Die die, int enclosing, unsigned long iaddr, const char *varname, Dwarf_Die *retDie){

  /*This function visits the children of a die in sequence, 
   *applying the action() function to each*/
  Dwarf_Attribute highattr;
  Dwarf_Attribute lowattr;
  Dwarf_Addr loval,hival;
  Dwarf_Bool has;
  Dwarf_Error error;
  Dwarf_Half tag; 
  
  loval = hival = 0x0;
  int enc = 0;

  dwarf_tag(die,&tag,&error);
 
  if( tag == DW_TAG_variable ||
      tag == DW_TAG_formal_parameter ){

    if(enclosing){
      char *name;
      dwarf_diename(die,&name,&error);
      if(!strncmp(name,varname,strlen(varname))){
        *retDie = die;
        return;
      }
    }
     
  }

  if( tag == DW_TAG_lexical_block || tag == DW_TAG_subprogram ){
    if( dwarf_lowpc(die,&loval,&error) == DW_DLV_OK  && dwarf_highpc(die,&hival,&error) == DW_DLV_OK
        && iaddr >=loval && iaddr <= hival ){ 
      enc = 1;
    }
  
  }
  
  Dwarf_Die kid;
  if( dwarf_child(die,&kid,&error) == DW_DLV_NO_ENTRY ){
    return;
  }
  DC_get_info_for_scoped_variable(kid, enc, iaddr,varname,retDie); 

  int chret;
  while( (chret = dwarf_siblingof(d,kid,&kid,&error)) != DW_DLV_NO_ENTRY &&
           chret != DW_DLV_ERROR){

    DC_get_info_for_scoped_variable(kid, enc, iaddr,varname,retDie); 

  }

  return;

}

static void DC_show_info_for_scoped_variable(Dwarf_Die die, unsigned long iaddr, const char *varname){

  Dwarf_Die v;
  v = (Dwarf_Die)0;
  
  DC_get_info_for_scoped_variable(die,0,iaddr,varname,&v);
  if( v != (Dwarf_Die)-1 ){

    show_all_attrs(die,0,NULL);

  }else{
    fprintf(stderr,"Couldn't get info for %s in scope at %lx\n",varname,iaddr);
  }

}


static void DC_show_vars_for_containing_pc_ranges(Dwarf_Die die, int enclosing, unsigned long iaddr){

  /*This function visits the children of a die in sequence, 
   *applying the action() function to each*/
  Dwarf_Attribute highattr;
  Dwarf_Attribute lowattr;
  Dwarf_Addr loval,hival;
  Dwarf_Bool has;
  Dwarf_Error error;
  Dwarf_Half tag; 
  
  loval = hival = 0x0;
  int enc = 0;

  dwarf_tag(die,&tag,&error);
 
  if( tag == DW_TAG_variable ||
      tag == DW_TAG_formal_parameter ){

    if(enclosing){
      char *name;
      dwarf_diename(die,&name,&error);
      fprintf(stderr,"%s, ",name);
    }
     
  }

  if( tag == DW_TAG_lexical_block || tag == DW_TAG_subprogram ){
    if( dwarf_lowpc(die,&loval,&error) == DW_DLV_OK  && dwarf_highpc(die,&hival,&error) == DW_DLV_OK
        && iaddr >=loval && iaddr <= hival ){ 
      enc = 1;
    }
  
  }
  
  Dwarf_Die kid;
  if( dwarf_child(die,&kid,&error) == DW_DLV_NO_ENTRY ){
    return;
  }
  DC_show_vars_for_containing_pc_ranges(kid, enc, iaddr); 

  int chret;
  while( (chret = dwarf_siblingof(d,kid,&kid,&error)) != DW_DLV_NO_ENTRY &&
           chret != DW_DLV_ERROR){

    DC_show_vars_for_containing_pc_ranges(kid, enc, iaddr); 

  }

  return;
}

static void DC_show_info_for_containing_pc_ranges(Dwarf_Die die, int enclosing, unsigned long iaddr){

  /*This function visits the children of a die in sequence, 
   *applying the action() function to each*/
  Dwarf_Attribute highattr;
  Dwarf_Attribute lowattr;
  Dwarf_Addr loval,hival;
  Dwarf_Bool has;
  Dwarf_Error error;
  Dwarf_Half tag; 
  
  loval = hival = 0x0;
  int enc = 0;

  dwarf_tag(die,&tag,&error);
 
  if( tag == DW_TAG_variable ||
      tag == DW_TAG_formal_parameter ){

    if(enclosing){
      char *name;
      dwarf_diename(die,&name,&error);
      fprintf(stderr,"%s, ",name);
      //show_all_attrs(die,0,NULL);
    }
     
  }

  if( tag == DW_TAG_lexical_block || tag == DW_TAG_subprogram ){
    if( dwarf_lowpc(die,&loval,&error) == DW_DLV_OK  && dwarf_highpc(die,&hival,&error) == DW_DLV_OK
        && iaddr >=loval && iaddr <= hival ){ 
      enc = 1;
      fprintf(stderr,"\n=================================\n");
      show_all_attrs(die,0,NULL);
      fprintf(stderr,"=================================\n");
    }
  
  }
  
  Dwarf_Die kid;
  if( dwarf_child(die,&kid,&error) == DW_DLV_NO_ENTRY ){
    return;
  }
  DC_show_info_for_containing_pc_ranges(kid, enc, iaddr); 
  //visit_die(kid,level+1,action,adata); 

  int chret;
  while( (chret = dwarf_siblingof(d,kid,&kid,&error)) != DW_DLV_NO_ENTRY &&
           chret != DW_DLV_ERROR){

    DC_show_info_for_containing_pc_ranges(kid, enc, iaddr); 
    //visit_die(kid,level+1,action,adata);

  }

  return;
}

static Dwarf_Die get_block_by_iaddr(unsigned long iaddr){
  Dwarf_Die cu = get_cu_by_iaddr(iaddr); 
   
}

unsigned long get_iaddr_of_file_line(const char *file, unsigned line){
 
  Dwarf_Unsigned cu_h_len;
  Dwarf_Half verstamp;
  Dwarf_Unsigned abbrev_offset;
  Dwarf_Half addrsize;
  Dwarf_Unsigned next_cu;
  Dwarf_Error error; 
  int found = 0;
  while( dwarf_next_cu_header(d,&cu_h_len,&verstamp,&abbrev_offset,&addrsize,&next_cu,&error) == DW_DLV_OK ){

    Dwarf_Die cu_die = NULL;
    int sibret;
 
    int dieno = 0; 
    while((sibret = 
           dwarf_siblingof(d,cu_die,&cu_die,&error)) != DW_DLV_NO_ENTRY &&
           sibret                                    != DW_DLV_ERROR){

      Dwarf_Signed cnt;
      Dwarf_Line *linebuf;
      int sres;
      if((sres = dwarf_srclines(cu_die, &linebuf, &cnt, &error)) == DW_DLV_OK){
        int i;
        for(i = 0; i < cnt; i++){
          char *lf;
          Dwarf_Unsigned ll;
          dwarf_linesrc(linebuf[i], &lf, &error); 
          dwarf_lineno(linebuf[i], &ll, &error);
          if(line == ll && strstr(lf,file)){
            Dwarf_Addr la;
            dwarf_lineaddr(linebuf[i], &la, &error);
            return la;
          }
          dwarf_dealloc(d, linebuf[i], DW_DLA_LINE);
        }
        dwarf_dealloc(d, linebuf, DW_DLA_LIST);
      }
    }
  }
  return 0xffffffffffffffff; 
}


void dump_dwarf_info(){

  Dwarf_Unsigned cu_h_len;
  Dwarf_Half verstamp;
  Dwarf_Unsigned abbrev_offset;
  Dwarf_Half addrsize;
  Dwarf_Unsigned next_cu;
  Dwarf_Error error; 


  int unit = 0;
  fprintf(stderr,"--Examining Compilation Units--\n");
  while( dwarf_next_cu_header(d,&cu_h_len,&verstamp,&abbrev_offset,&addrsize,&next_cu,&error) == DW_DLV_OK ){

    Dwarf_Die cu_die = NULL;
    int sibret;
 
    int dieno = 0; 
    while((sibret = 
           dwarf_siblingof(d,cu_die,&cu_die,&error)) != DW_DLV_NO_ENTRY &&
           sibret                                    != DW_DLV_ERROR){
  
      Dwarf_Half cu_tag; 
      dwarf_tag(cu_die,&cu_tag,&error);

 
      visit_die(cu_die,0,show_all_attrs,NULL);
    
    };
    if(!sibret == DW_DLV_NO_ENTRY){
      fprintf(stderr,"--Error Examining DIEs--\n");
      exit(1);
    }
  }

}


void show_scopes_by_file_line(char *fileline_fn, int fileline_ln){
  unsigned long a = get_iaddr_of_file_line(fileline_fn,fileline_ln);
  reset_cu_header_info();
  Dwarf_Die cu = get_cu_by_iaddr(a);
  reset_cu_header_info();
  if(a != 0xffffffffffffffff && (long int)cu != -1){
    DC_show_info_for_containing_pc_ranges(cu,0,a);
  }else{
    fprintf(stderr,"I can't find address %x\n",a);
  }
  fprintf(stderr,"\n");
}

void show_vars_by_scope_addr(void *addr){
  unsigned long a = (unsigned long)addr;
  reset_cu_header_info();
  Dwarf_Die cu = get_cu_by_iaddr(a);
  reset_cu_header_info();
  if(a != 0xffffffffffffffff && (long int)cu != -1){
    DC_show_vars_for_containing_pc_ranges(cu,0,a);
  }else{
    fprintf(stderr,"I can't find address %lx\n",a);
  }
  fprintf(stderr,"\n");
}

void show_scopes_by_addr(void *addr){
  unsigned long a = (unsigned long)addr;
  reset_cu_header_info();
  Dwarf_Die cu = get_cu_by_iaddr(a);
  reset_cu_header_info();
  if(a != 0xffffffffffffffff && (long int)cu != -1){
    DC_show_info_for_containing_pc_ranges(cu,0,a);
  }else{
    fprintf(stderr,"I can't find address %x\n",a);
  }
  fprintf(stderr,"\n");
}

long get_location_of_scoped_variable(void *addr,const char * varname){

  Dwarf_Error error;
  unsigned long a = (unsigned long)addr;

  reset_cu_header_info();

  Dwarf_Die cu = get_cu_by_iaddr(a);

  reset_cu_header_info();

  if(a != 0xffffffffffffffff && (long int)cu != -1){

    Dwarf_Die v;
    v = (Dwarf_Die)0; 
    DC_get_info_for_scoped_variable(cu,0,a,varname,&v);
    if( v != (Dwarf_Die)-1 ){

      Dwarf_Attribute loc;
      dwarf_attr(v,DW_AT_location,&loc,&error);

      DC_location dloc;
      DC_get_location_attr_value(loc,&dloc);
      return dloc.offset;
    }

  }else{

    fprintf(stderr,"Can't find \"%s\" in scope defined by <%x>\n",varname, a);

  }

  fprintf(stderr,"\n");

}

void show_info_for_scoped_variable(void *addr,const char * varname){

  unsigned long a = (unsigned long)addr;

  reset_cu_header_info();

  Dwarf_Die cu = get_cu_by_iaddr(a);

  reset_cu_header_info();

  if(a != 0xffffffffffffffff && (long int)cu != -1){

    DC_show_info_for_scoped_variable(cu,a,varname);

  }else{

    fprintf(stderr,"Can't find \"%s\" in scope defined by <%x>\n",varname, a);

  }

  fprintf(stderr,"\n");
}



#ifdef DWARF_CLIENT_LIB
int opendwarf(char *argv){
#else
int main(int argc, char **argv){
#endif

#ifdef DWARF_CLIENT_LIB
  char *fname = argv;
#else
  char *fname = argv[1];
#endif

#ifndef DWARF_CLIENT_LIB

  int getCU = 0;
  int getAddrOfFileLine = 0;
  const char *fileline_fn = NULL;
  int fileline_ln = 0;
  int dump = 0;
  char opt;
  const char *optString = "cgf:l:e:ph";
  while( (opt = getopt(argc,argv,optString) ) != -1 ){
    switch(opt){
      case 'c':
        getCU = 1;
        break;
      case 'g':
        getAddrOfFileLine = 1;
        break;
      case 'f':
        fileline_fn = strdup(optarg);
        break;
      case 'l':
        fileline_ln = atoi(optarg);
        break;
      case 'e':
        fname = strdup(optarg); 
        break;
      case 'p':
        dump = 1;
        break;
      case 'h':
        fprintf(stderr,"Usage: ./dwarfinfo -e <executable> [Option(s)]\n"); 
        fprintf(stderr,"\t-e Specify executable\n");
        fprintf(stderr,"\t-g Get instruction address of file and line (requires -f and -l)\n");
        fprintf(stderr,"\t-c Show enclosing scopes and variables in each for file and line (requires -f and -l)\n");
        fprintf(stderr,"\t-p Dump dwarf information for executable\n");
        fprintf(stderr,"\t-f Specify source file\n");
        fprintf(stderr,"\t-l Specify source line\n");
        fprintf(stderr,"\t-h Help\n");

      default:
        break;
    };
  }
#endif

  int fd = open(fname,O_RDONLY);
  if(fd == -1){exit(-1);} 
  dwarf_init(fd,DW_DLC_READ,NULL,NULL,&d,&e) ;

#ifndef DWARF_CLIENT_LIB
  unsigned long a;
  if(getAddrOfFileLine){
    a = get_iaddr_of_file_line(fileline_fn,fileline_ln);
    fprintf(stderr,"%x\n",a);
  }

  if(getCU && a != 0xffffffffffffffff){
    reset_cu_header_info();
    Dwarf_Die cu = get_cu_by_iaddr(a);
    reset_cu_header_info();
    DC_show_info_for_containing_pc_ranges(cu,0,a);
    fprintf(stderr,"\n");
  }

  if(dump){
    reset_cu_header_info();
    dump_dwarf_info();
  }

#endif

 
}
