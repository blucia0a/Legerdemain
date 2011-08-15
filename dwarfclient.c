#include <libdwarf.h>
#include <dwarf.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "stack.h"
#include "dwarfclient.h"

void decode_location(Dwarf_Locdesc *locationList, Dwarf_Signed listLength, long *offset, long *init_val, int *frameRel){

  /*Location Decoding Code from http://ns.dyninst.org/coverage/dyninstAPI/src/parseDwarf.C.gcov.html*/
  Stack *opStack = (Stack*)malloc(sizeof(opStack));
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


void show_all_attrs(Dwarf_Die die){
    Dwarf_Error error;
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
          fprintf(stderr,"\tAttr: %s => ",sattr);
        }else{
          fprintf(stderr,"\tCouldn't Get Attr Type!\n"); 
          continue;
        }
        Dwarf_Half form;
        if(dwarf_whatform(atlist[i],&form,&error) == DW_DLV_OK){
          const char *formname;
          dwarf_get_FORM_name(form,&formname);
          #ifdef SHOWFORM
          fprintf(stderr,"[%s] ",formname);
          #endif
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
              if(attr != DW_AT_location &&
                 attr != DW_AT_data_member_location){ 
                fprintf(stderr,"Unsupported\n");
              }else{
          
                Dwarf_Locdesc *locationList;
                Dwarf_Signed listLength;
                int ret = dwarf_loclist( atlist[i], &locationList, &listLength, NULL );
                int frameRel = 0;
                long offset = 0;
                decode_location(locationList,listLength,&offset,NULL,&frameRel);
                Dwarf_Block *val;
                dwarf_formblock(atlist[i],&val, &error);
                //fprintf(stderr,"Block: [Len=%u, ptr=%x, loclist=%d, off=%u]",val->bl_len,val->bl_data,val->bl_from_loclist,val->bl_section_offset); 
                unsigned char data[val->bl_len + 1];
                memcpy(data,val->bl_data,val->bl_len);
                data[val->bl_len] = 0;
                fprintf(stderr,"[");
                int i;
                for(i = 0; i < val->bl_len; i++){
                  fprintf(stderr," %hhx",data[i]);
                }
                fprintf(stderr," ]");
                fprintf(stderr," %s:",frameRel ? "FP Offset" : "Address");
                fprintf(stderr," %ld\n",offset);

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
              /*{Dwarf_Addr addr;
              dwarf_formaddr(atlist[i],&addr,&error); 
              fprintf(stderr,"%x\n",addr);
              fprintf(stderr,"%s\n",*(char*)addr);}
              break;*/
              
            
            default:
              fprintf(stderr,"Unhandled Attribute Form!\n");
              break;
              
          };
        }
        //dwarf_dealloc(d, atlist, DW_DLA_ATTR);
      }
    } 

}

void visit_die(Dwarf_Die die, unsigned int level){

  /*This function visits the children of a die in sequence, 
   *dumping their attributes*/
  /*First: Dump this die's [level]tag and all attributes*/
  Dwarf_Error error;
  Dwarf_Half tag; 
 
  dwarf_tag(die,&tag,&error);

  const char *stag;
  dwarf_get_TAG_name(tag,&stag); 
  fprintf(stderr,"[%u]%s\n",level,stag);
  show_all_attrs(die);
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

  Dwarf_Die kid;
  if( dwarf_child(die,&kid,&error) == DW_DLV_NO_ENTRY ){
    return;
  }
  visit_die(kid,level+1); 

  int chret;


  while( (chret = dwarf_siblingof(d,kid,&kid,&error)) != DW_DLV_NO_ENTRY &&
           chret != DW_DLV_ERROR){

    visit_die(kid,level+1);

  }

  return;
  
}

int examine_cu(Dwarf_Die die){

  Dwarf_Error error;
  Dwarf_Die kid;
  if( dwarf_child(die,&kid,&error) == DW_DLV_NO_ENTRY ){
    return 0;
  }

  int chret;
  while( (chret = dwarf_siblingof(d,kid,&kid,&error)) != DW_DLV_NO_ENTRY &&
         chret != DW_DLV_ERROR){

    Dwarf_Half tag; 
    dwarf_tag(kid,&tag,&error);

    const char *stag;
    dwarf_get_TAG_name(tag,&stag); 
    fprintf(stderr,"Tag: %s\n",stag);
    show_all_attrs(kid); 

  }
  return 1;

}

#ifdef DWARF_CLIENT_LIB
int getdwarfdata(char *argv){
#else
int main(int argc, char **argv){
#endif

#ifdef DWARF_CLIENT_LIB
  char *fname = argv;
#else
  char *fname = argv[1];
#endif
  fprintf(stderr,"Opening binary: %s\n",fname);
  int fd = open(fname,O_RDONLY);
  if(fd == -1){exit(-1);} 
  dwarf_init(fd,DW_DLC_READ,NULL,NULL,&d,&e) ;

  Dwarf_Unsigned cu_h_len;
  Dwarf_Half verstamp;
  Dwarf_Unsigned abbrev_offset;
  Dwarf_Half addrsize;
  Dwarf_Unsigned next_cu;
  Dwarf_Error error; 

  ;

  int unit = 0;
  fprintf(stderr,"--Examining Compilation Units--\n");
  while( dwarf_next_cu_header(d,&cu_h_len,&verstamp,&abbrev_offset,&addrsize,&next_cu,&error) == DW_DLV_OK ){

    fprintf(stderr,"==Unit %d== Version: %u. Addrsize: %u.\n",unit++,verstamp,addrsize);
    Dwarf_Die cu_die = NULL;
    int sibret;
 
    int dieno = 0; 
    fprintf(stderr,"--Examining Top-Level DIEs--\n");
    while((sibret = 
           dwarf_siblingof(d,cu_die,&cu_die,&error)) != DW_DLV_NO_ENTRY &&
           sibret                                    != DW_DLV_ERROR){
  
      Dwarf_Half cu_tag; 
      dwarf_tag(cu_die,&cu_tag,&error);
  
      const char *tag;
      dwarf_get_TAG_name(cu_tag,&tag); 
      fprintf(stderr,"=Die # %d=  Tag: %s\n",dieno,tag);
 
      visit_die(cu_die,0);
    
    };
    if(sibret == DW_DLV_NO_ENTRY){
      fprintf(stderr,"--Done Examining DIEs--\n");
    }else{
      fprintf(stderr,"--Error Examining DIEs--\n");
      exit(1);
    }
  }

  fprintf(stderr,"--Done Examining Compilation Units--\n");
 
}
