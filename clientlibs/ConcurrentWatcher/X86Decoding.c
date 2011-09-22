#include <libdis.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define __USE_GNU
#include <ucontext.h>

void setupX86Decoder(){

  x86_init(opt_none,NULL,NULL);

}

int getRegisterEntry( x86_reg_t reg ){

  switch( reg.id ){
    case 1:
      return REG_RAX;
    case 2:
      return REG_RCX;
    case 3:
      return REG_RDX;
    case 4:
      return REG_RBX;
    case 5:
      return REG_RSP;
    case 6:
      return REG_RBP;
    case 7:
      return REG_RSI;
    case 8:
      return REG_RDI;
    default:
      return -1;
  }
}

unsigned long getRegisterValue( x86_reg_t r, ucontext_t *ctx ){

  int regEntry = getRegisterEntry(r);
  if( regEntry != -1 ){
    return ctx->uc_mcontext.gregs[regEntry];
  }else{
    return 0x0;
  }

}

void **getOperandAddresses( void *ad, ucontext_t *ctx, void **opAddrs ){

  x86_insn_t insn;
  int size = x86_disasm(((char *)ad)+1,sizeof(ad),0,0,&insn);

  /*
  char *c;
  for( c = ad; c < ((char*)ad) + sizeof(ad); c++ ){
    fprintf(stderr,"%02hhx ",*c);
  }
  */

  char line[1000];
  if( size ){/*Successfully Decoded*/

    int whichop = 0;
    x86_oplist_t *op = insn.operands; 
    while(op != NULL){

      switch(op->op.type){

        case op_register:
          //fprintf(stderr,"reg: %s \n", op->op.data.reg.name);
          //fprintf(stderr,"REG[%s] = %lx\n",op->op.data.reg.name,getRegisterValue(op->op.data.reg,ctx));
          opAddrs[whichop++] = (void*)getRegisterValue(op->op.data.reg,ctx); 

          break;

        case op_immediate:
          //fprintf(stderr,"Immediate!\n");
          break;

        case op_relative_near:
          //fprintf(stderr,"relnear: %hhd \n", op->op.data.relative_near);
          break;

        case op_relative_far:
          //fprintf(stderr,"relfar: %d \n", op->op.data.relative_far);
          break;

        case op_absolute:
          //fprintf(stderr,"seg: %hu off: %u\n", op->op.data.absolute.segment, op->op.data.absolute.offset.off32);
          break;

        case op_expression:
          //fprintf(stderr,"scale: %u index: %s base: %s disp: %x\n",op->op.data.expression.scale,op->op.data.expression.index.name,op->op.data.expression.base.name,op->op.data.expression.disp * (op->op.data.expression.disp_sign == 0 ? 1: -1));
          /*fprintf(stderr,"(REG[%s]) + %u * (REG[%s]) = %lx\n",
                          op->op.data.expression.base.name,
                          op->op.data.expression.scale,
                          op->op.data.expression.index.name,
                          getRegisterValue(op->op.data.expression.index,ctx)*
                          op->op.data.expression.scale + 
                          getRegisterValue(op->op.data.expression.base,ctx));*/
          opAddrs[whichop++] = (void*)(getRegisterValue(op->op.data.expression.index,ctx)* 
                               op->op.data.expression.scale + 
                               getRegisterValue(op->op.data.expression.base,ctx));
          break;

        case op_offset:
          break;

        default:
          break;
      }; 

      op = op->next; 

    } 

    //x86_format_insn(&insn,line,100,intel_syntax);
    //fprintf(stderr,"%s\n",line);

  }else{

    fprintf(stderr,"Invalid insns\n");

  }

}
