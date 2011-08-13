#include <libdwarf.h>
#include <dwarf.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>

Dwarf_Debug d;
Dwarf_Error e;

examine_cu(Dwarf_Die die){

  Dwarf_Error error;
  Dwarf_Die kid;
  if( dwarf_child(die,&kid,&error) == DW_DLV_NO_ENTRY ){
    return;
  }

  int chret;
  while( (chret = dwarf_siblingof(d,kid,&kid,&error)) != DW_DLV_NO_ENTRY &&
         chret != DW_DLV_ERROR){

    Dwarf_Half tag; 
    dwarf_tag(kid,&tag,&error);

    const char *stag;
    dwarf_get_TAG_name(tag,&stag); 
    fprintf(stderr,"Tag: %s\n",stag);
    
    Dwarf_Unsigned atcnt;
    Dwarf_Attribute *atlist;
    int errv;
    if((errv = dwarf_attrlist(kid, &atlist, &atcnt, &error)) == DW_DLV_OK){
      int i;
      for(i = 0; i < atcnt; i++){
        Dwarf_Half attr;
        if(dwarf_whatattr(atlist[i],&attr,&error) == DW_DLV_OK){
          const char *sattr;
          dwarf_get_AT_name(attr,&sattr); 
          fprintf(stderr,"\tAttr: %s\n",sattr);
        }
        Dwarf_Half form;
        if(dwarf_whatform(atlist[i],&form,&error) == DW_DLV_OK){
          const char *formname;
          dwarf_get_FORM_name(form,&formname);
          fprintf(stderr,"\t\tForm: %s\n",formname);
          switch(form){
            case DW_FORM_ref1:
            case DW_FORM_ref2:
            case DW_FORM_ref4:
            case DW_FORM_ref8:
            case DW_FORM_ref_udata:
              {Dwarf_Off offset;
              dwarf_formref(atlist[i],&offset,&error); 
              fprintf(stderr,"\t\tOffset: %x\n",offset);}
              break;
            case DW_FORM_ref_addr:
              {Dwarf_Off offset;
              dwarf_global_formref(atlist[i],&offset,&error); 
              fprintf(stderr,"\t\tOffset: %x\n",offset);}
              break;
            case DW_FORM_addr:
              {Dwarf_Addr addr;
              dwarf_formaddr(atlist[i],&addr,&error); 
              fprintf(stderr,"\t\tAddress: %x\n",addr);}
              break;
            case DW_FORM_flag:
              {Dwarf_Bool flag;
              dwarf_formflag(atlist[i],&flag,&error); 
              fprintf(stderr,"\t\tBoolean Value: %s\n",flag ? "True" : "False");}
              break;
            case DW_FORM_udata:
            case DW_FORM_data1:
            case DW_FORM_data2:
            case DW_FORM_data4:
            case DW_FORM_data8:
              {Dwarf_Unsigned val;
              dwarf_formudata(atlist[i],&val,&error); 
              fprintf(stderr,"\t\tValue: %u\n",val);}
              break;
            case DW_FORM_sdata:
              {Dwarf_Signed val;
              dwarf_formsdata(atlist[i],&val,&error); 
              fprintf(stderr,"\t\tValue: %d\n",val);}
              break;
            case DW_FORM_block:
            case DW_FORM_block1:
              {Dwarf_Signed val;
              fprintf(stderr,"\t\tUnsupported\n");}
              break;
            case DW_FORM_string:
              {char *val;
              dwarf_formstring(atlist[i],&val,&error); 
              fprintf(stderr,"\t\tValue: %s\n",val);}
              break;
           
            
            
          };
        }
        //dwarf_dealloc(d, atlist, DW_DLA_ATTR);
      }
    } 
  }

}

int main(int argc, char **argv){
  char *fname = argv[1];
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
  
      examine_cu(cu_die); 
    
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
