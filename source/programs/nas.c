/*
 * User program: 16-bit Intel-8086 Assembler
 */

#include "types.h"
#include "ulib/ulib.h"

#define EOF 0xFFE0 /* End Of File */

/* Code generation vars */
uint cg_origin = 0x0000; /* Absolute pos in segment */

/* Symbol types */
enum S_TYPE {
  S_LABEL,  /* Label */
  S_DATAW,  /* Data word */
  S_DATAB   /* Data byte */
};

/* Symbols table */
#define S_MAX 32
struct symbol {
  uchar name[8];
  uint  type;  /* See enum S_TYPE */
  uint  value;
} s_table[S_MAX];

/* Operand types */
enum O_TYPE {
  O_ANY,  /* unknown */
  O_RB,   /* Register byte */
  O_RW,   /* Register word */
  O_RD,   /* Register double */
  O_RMW,  /* Memory word in a register */
  O_MW,   /* Memory word immediate */
  O_IW    /* Immediate */
};

/* Registers */
enum R_DEF {
  R_ANY, /* Unknown */
  R_AX,
  R_CX,
  R_DX,
  R_BX,
  R_SP,
  R_BP,
  R_SI,
  R_DI,
  R_COUNT
};

/* CONST Registers data */
struct rdata {
  uchar name[5];
  uchar type;
  uchar encoding;
} r_data[R_COUNT] =
{
  "NO", O_ANY, 0x00,
  "ax", O_RW,  0x00,
  "cx", O_RW,  0x01,
  "dx", O_RW,  0x02,
  "bx", O_RW,  0x03,
  "sp", O_RW,  0x04,
  "bp", O_RW,  0x05,
  "si", O_RW,  0x06,
  "di", O_RW,  0x07,
};

/* Instruction id */
enum I_DEF {
  I_PUSH,
  I_POP,
  I_MOV_RIW,
  I_MOV_RRW,
  I_MOV_RMW,
  I_MOV_MRW,
  I_MOV_RRMW,
  I_MOV_RMRW,
  I_CMP_RIW,
  I_CMP_RRW,
  I_CMP_RMW,
  I_CMP_MRW,
  I_CMP_RRMW,
  I_CMP_RMRW,
  I_RET,
  I_RETF,
  I_INT,
  I_CALL,
  I_JMP,
  I_JE,
  I_JNE,
  I_JG,
  I_JGE,
  I_JL,
  I_JLE,
  I_JC,
  I_JNC,
  I_ADD_RIW,
  I_ADD_RRW,
  I_ADD_RMW,
  I_ADD_MRW,
  I_ADD_RRMW,
  I_ADD_RMRW,
  I_SUB_RIW,
  I_SUB_RRW,
  I_SUB_RMW,
  I_SUB_MRW,
  I_SUB_RRMW,
  I_SUB_RMRW,
  I_MUL_RW,
  I_DIV_RW,
  I_NOT_RW,
  I_AND_RIW,
  I_AND_RRW,
  I_AND_RMW,
  I_AND_MRW,
  I_AND_RRMW,
  I_AND_RMRW,
  I_OR_RIW,
  I_OR_RRW,
  I_OR_RMW,
  I_OR_MRW,
  I_OR_RRMW,
  I_OR_RMRW,
  I_COUNT
};

/* CONST Instructions data */
#define I_MAX_OPS 3 /* Max instruction operands */
struct idata {
  uchar mnemonic[6];
  uchar opcode;
  uchar nops;     /* Number of operands */
  uchar op_type[I_MAX_OPS]; /* Type of operands */
  uint  op_value[I_MAX_OPS]; /* Restricted value */
} i_data[I_COUNT] =
{
  "push", 0x50, 1, O_RW,  O_ANY, O_ANY,   R_ANY, 0,     0,
  "pop",  0x58, 1, O_RW,  O_ANY, O_ANY,   R_ANY, 0,     0,
  "mov",  0xB8, 2, O_RW,  O_IW,  O_ANY,   R_ANY, 0,     0,
  "mov",  0x89, 2, O_RW,  O_RW,  O_ANY,   R_ANY, R_ANY, 0,
  "mov",  0x8B, 2, O_RW,  O_MW,  O_ANY,   R_ANY, 0,     0,
  "mov",  0x89, 2, O_MW,  O_RW,  O_ANY,   0,     R_ANY, 0,
  "mov",  0x8B, 2, O_RW,  O_RMW, O_ANY,   R_ANY, R_BX,  0,
  "mov",  0x89, 2, O_RMW, O_RW,  O_ANY,   R_BX,  R_ANY, 0,
  "cmp",  0x81, 2, O_RW,  O_IW,  O_ANY,   R_ANY, 0,     0,
  "cmp",  0x39, 2, O_RW,  O_RW,  O_ANY,   R_ANY, R_ANY, 0,
  "cmp",  0x3B, 2, O_RW,  O_MW,  O_ANY,   R_ANY, 0,     0,
  "cmp",  0x39, 2, O_MW,  O_RW,  O_ANY,   0,     R_ANY, 0,
  "cmp",  0x3B, 2, O_RW,  O_RMW, O_ANY,   R_ANY, R_BX,  0,
  "cmp",  0x39, 2, O_RMW, O_RW,  O_ANY,   R_BX,  R_ANY, 0,
  "ret",  0xC3, 0, O_ANY, O_ANY, O_ANY,   0,     0,     0,
  "retf", 0xCB, 0, O_ANY, O_ANY, O_ANY,   0,     0,     0,
  "int",  0xCD, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "call", 0xE8, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jmp",  0xE9, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "je",   0x74, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jne",  0x75, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jg",   0x7F, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jge",  0x7D, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jl",   0x7C, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jle",  0x7E, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jc",   0x72, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "jnc",  0x73, 1, O_IW,  O_ANY, O_ANY,   0,     0,     0,
  "add",  0x81, 2, O_RW,  O_IW,  O_ANY,   R_ANY, 0,     0,
  "add",  0x01, 2, O_RW,  O_RW,  O_ANY,   R_ANY, R_ANY, 0,
  "add",  0x03, 2, O_RW,  O_MW,  O_ANY,   R_ANY, 0,     0,
  "add",  0x01, 2, O_MW,  O_RW,  O_ANY,   0,     R_ANY, 0,
  "add",  0x03, 2, O_RW,  O_RMW, O_ANY,   R_ANY, R_BX,  0,
  "add",  0x01, 2, O_RMW, O_RW,  O_ANY,   R_BX,  R_ANY, 0,
  "sub",  0x81, 2, O_RW,  O_IW,  O_ANY,   R_ANY, 0,     0,
  "sub",  0x29, 2, O_RW,  O_RW,  O_ANY,   R_ANY, R_ANY, 0,
  "sub",  0x2B, 2, O_RW,  O_MW,  O_ANY,   R_ANY, 0,     0,
  "sub",  0x29, 2, O_MW,  O_RW,  O_ANY,   0,     R_ANY, 0,
  "sub",  0x2B, 2, O_RW,  O_RMW, O_ANY,   R_ANY, R_BX,  0,
  "sub",  0x29, 2, O_RMW, O_RW,  O_ANY,   R_BX,  R_ANY, 0,
  "mul",  0xF7, 2, O_RW,  O_ANY, O_ANY,   R_ANY, 0,     0,
  "div",  0xF7, 2, O_RW,  O_ANY, O_ANY,   R_ANY, 0,     0,
  "not",  0xF7, 2, O_RW,  O_ANY, O_ANY,   R_ANY, 0,     0,
  "and",  0x81, 2, O_RW,  O_IW,  O_ANY,   R_ANY, 0,     0,
  "and",  0x21, 2, O_RW,  O_RW,  O_ANY,   R_ANY, R_ANY, 0,
  "and",  0x23, 2, O_RW,  O_MW,  O_ANY,   R_ANY, 0,     0,
  "and",  0x21, 2, O_MW,  O_RW,  O_ANY,   0,     R_ANY, 0,
  "and",  0x23, 2, O_RW,  O_RMW, O_ANY,   R_ANY, R_BX,  0,
  "and",  0x21, 2, O_RMW, O_RW,  O_ANY,   R_BX,  R_ANY, 0,
  "or",   0x81, 2, O_RW,  O_IW,  O_ANY,   R_ANY, 0,     0,
  "or",   0x09, 2, O_RW,  O_RW,  O_ANY,   R_ANY, R_ANY, 0,
  "or",   0x0B, 2, O_RW,  O_MW,  O_ANY,   R_ANY, 0,     0,
  "or",   0x09, 2, O_MW,  O_RW,  O_ANY,   0,     R_ANY, 0,
  "or",   0x0B, 2, O_RW,  O_RMW, O_ANY,   R_ANY, R_BX,  0,
  "or",   0x09, 2, O_RMW, O_RW,  O_ANY,   R_BX,  R_ANY, 0,
};

/* Symbols reference table */
#define S_MAX_REF 32
struct s_ref {
  uint   offset;
  uchar  symbol;
  uchar  operand;
  uchar  instr_id;
  struct idata instruction;
} s_ref[S_MAX_REF];

/*
 * Encode and write instruction to buffer
 */
uint encode_instruction(uchar* buffer, uint offset, uchar id, uint* op)
{
  switch(id) {

  case I_RETF:
  case I_RET: {
    buffer[offset++] = i_data[id].opcode;
    debugstr(": %2x", buffer[offset-1]);
    break;
  }

  case I_PUSH:
  case I_POP: {
    buffer[offset++] =
      i_data[id].opcode + r_data[op[0]].encoding;

    debugstr(": %2x", buffer[offset-1]);
    break;
  }

  case I_INT: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = op[0];
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_JNE:
  case I_JG:
  case I_JGE:
  case I_JL:
  case I_JLE:
  case I_JC:
  case I_JNC:
  case I_JE: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = (op[0]-cg_origin-(offset+1));
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_CALL:
  case I_JMP: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = (op[0]-cg_origin-(offset+2))&0xFF;
    buffer[offset++] = (op[0]-cg_origin-(offset+2))>>8;
    debugstr(": %2x %2x %2x", buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_OR_RIW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(0x01<<3)|r_data[op[0]].encoding;
    buffer[offset++] = op[1] & 0xFF;
    buffer[offset++] = op[1] >> 8;
    debugstr(": %2x %2x %2x %2x", buffer[offset-4],
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_AND_RIW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(0x04<<3)|r_data[op[0]].encoding;
    buffer[offset++] = op[1] & 0xFF;
    buffer[offset++] = op[1] >> 8;
    debugstr(": %2x %2x %2x %2x", buffer[offset-4],
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_NOT_RW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(0x02<<3)|r_data[op[0]].encoding;
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_MUL_RW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(0x04<<3)|r_data[op[0]].encoding;
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_DIV_RW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(0x06<<3)|r_data[op[0]].encoding;
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_SUB_RIW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(0x05<<3)|r_data[op[0]].encoding;
    buffer[offset++] = op[1] & 0xFF;
    buffer[offset++] = op[1] >> 8;
    debugstr(": %2x %2x %2x %2x", buffer[offset-4],
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_ADD_RIW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|r_data[op[0]].encoding;
    buffer[offset++] = op[1] & 0xFF;
    buffer[offset++] = op[1] >> 8;
    debugstr(": %2x %2x %2x %2x", buffer[offset-4],
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_CMP_RIW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(0x07<<3)|r_data[op[0]].encoding;
    buffer[offset++] = op[1] & 0xFF;
    buffer[offset++] = op[1] >> 8;
    debugstr(": %2x %2x %2x %2x", buffer[offset-4],
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_MOV_RIW: {
    buffer[offset++] = i_data[id].opcode + r_data[op[0]].encoding;
    buffer[offset++] = op[1] & 0xFF;
    buffer[offset++] = op[1] >> 8;
    debugstr(": %2x %2x %2x",
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_OR_RRW:
  case I_AND_RRW:
  case I_SUB_RRW:
  case I_ADD_RRW:
  case I_CMP_RRW:
  case I_MOV_RRW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0xC0|(r_data[op[1]].encoding<<3)|r_data[op[0]].encoding;
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_OR_RMW:
  case I_AND_RMW:
  case I_SUB_RMW:
  case I_ADD_RMW:
  case I_CMP_RMW:
  case I_MOV_RMW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0x06|(r_data[op[0]].encoding<<3);
    buffer[offset++] = op[1] & 0xFF;
    buffer[offset++] = op[1] >> 8;
    debugstr(": %2x %2x %2x %2x", buffer[offset-4],
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_OR_MRW:
  case I_AND_MRW:
  case I_SUB_MRW:
  case I_ADD_MRW:
  case I_CMP_MRW:
  case I_MOV_MRW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0x06|(r_data[op[1]].encoding<<3);
    buffer[offset++] = op[0] & 0xFF;
    buffer[offset++] = op[0] >> 8;
    debugstr(": %2x %2x %2x %2x", buffer[offset-4],
      buffer[offset-3], buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_OR_RRMW:
  case I_AND_RRMW:
  case I_SUB_RRMW:
  case I_ADD_RRMW:
  case I_CMP_RRMW:
  case I_MOV_RRMW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0x07|(r_data[op[0]].encoding<<3);
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  case I_OR_RMRW:
  case I_AND_RMRW:
  case I_SUB_RMRW:
  case I_ADD_RMRW:
  case I_CMP_RMRW:
  case I_MOV_RMRW: {
    buffer[offset++] = i_data[id].opcode;
    buffer[offset++] = 0x07|(r_data[op[1]].encoding<<3);
    debugstr(": %2x %2x", buffer[offset-2], buffer[offset-1]);
    break;
  }

  default: {
    debugstr(": Instruction not found\n\r");
    return ERROR_NOT_FOUND;
  }

  };

  return offset;
}

/*
 * Encode and write data to buffer
 */
uint encode_data(uchar* buffer, uint offset, uchar type, uint value)
{
  switch(type) {

  case S_DATAW: {
    buffer[offset++] = value & 0xFF;
    buffer[offset++] = value >> 8;
    break;
  }

  case S_DATAB: {
    buffer[offset++] = value & 0xFF;
    break;
  }

  default: {
   debugstr(": Unknown data type\n\r");
   break;
  }

  };
  return offset;
}

/*
 * Given a file and offset, returns a file line and offset to next line start
 */
static uint read_line(uchar* buff, uint buff_size, uchar* file, uint offset)
{
  uint i = 0;
  uint readed = 0;

  /* Clear buffer and read */
  memset(buff, 0, buff_size);
  readed = read_file(buff, file, offset, buff_size);

  /* Return on error */
  if(readed >= ERROR_ANY) {
    return readed;
  }

  /* Return EOF in case */
  if(readed == 0) {
    debugstr("EOF\n\r");
    return EOF;
  }

  /* Clamp buff to a single line */
  for(i=0; i<readed; i++) {

    /* Ignore '\r' */
    if(buff[i] == '\r') {
      buff[i] = ' ';
    }

    /* If new line, clamp buff to a single line
     * and return offset of next char */
    if(buff[i] == '\n') {
      buff[i] = 0;
      return offset+i+1;
    }

    /* If last line, clamp buff */
    if(i==readed-1 && readed!=buff_size) {
      buff[i+1] = 0;
      return offset+i+1;
    }
  }

  /* Line is too long */
  return ERROR_ANY;
}

/*
 * Given a text line, returns token count and token start vectors
 */
static uint tokenize_line(uchar* buff, uint size, uchar* tokv[], uint max_tok)
{
  uint tokc = 0;
  tokv[0] = buff;
  buff[size-1] = 0;

  while(*buff && tokc<max_tok) {
    /* Remove initial separator symbols */
    while(*buff==' ' || *buff=='\t' || *buff==',') {
      *buff = 0;
      buff++;
    }

    /* Current token starts here */
    tokv[tokc] = buff;
    while(*buff && *buff != ' '  &&
                   *buff != '\t' &&
                   *buff != ','  &&
                   *buff != ';') {
      buff++;
    }

    /* Skip comments */
    if(tokv[tokc][0] == ';') {
      tokv[tokc][0] = 0;
      return tokc;
    }

    /* Next token */
    if(tokv[tokc][0] != 0) {
      tokc++;
    }
  }

  return tokc;
}

/*
 * Find or add symbol to table
 */
 uint find_or_add_symbol(uchar* name)
 {
   uint s = 0;
   for(s=0; s<S_MAX; s++) {
     if(s_table[s].name[0] == 0) {
       /* Not found, so add */
       strcpy_s(s_table[s].name, name, sizeof(s_table[s].name));
       return s;
     } else if(!strcmp(s_table[s].name, name)) {
       /* Found, return index */
       return s;
     }
   }
   return ERROR_NOT_FOUND;
 }

/*
 * Find symbol in table and return index
 */
uint find_symbol(uchar* name)
{
  uint s = 0;
  for(s=0; s<S_MAX; s++) {
    if(s_table[s].name[0] == 0) {
      /* Not found and no more symbols */
      break;
    } else if(!strcmp(s_table[s].name, name)) {
      /* Found */
      return s;
    }
  }
  return ERROR_NOT_FOUND;
}

/*
 * Append a symbol reference to symbol references table
 */
void append_symbol_ref(uchar symbol, uchar operand, uint offset,
  uchar instr_id, struct idata* instruction)
{
  uint r = 0;
  for(r=0; r<S_MAX_REF; r++) {
    if(s_ref[r].offset == 0) {

      /* First empty, add here */
      s_ref[r].symbol = symbol;
      s_ref[r].operand = operand;
      s_ref[r].offset = offset;
      s_ref[r].instr_id = instr_id;

      memcpy(&s_ref[r].instruction, instruction,
        sizeof(s_ref[r].instruction));

      break;
    }
  }
}

/*
 * Find matching instruction and return id
 */
uchar find_instruction(struct idata* ci)
{
  uchar i=0, id=0;

  for(id=0; id<I_COUNT; id++) {
    /* Check same name and number of args */
    if(!strcmp(ci->mnemonic, i_data[id].mnemonic) && ci->nops==i_data[id].nops) {

      /* Assume match */
      uint match = 1;
      for(i=0; i<ci->nops; i++) {

        /* Unless some arg is not the same type */
        if(ci->op_type[i] != i_data[id].op_type[i]) {
          match = 0;
          break;
        }

        /* Or a non matching specific value is required */
        if(i_data[id].op_value[i]!=0 &&
          i_data[id].op_value[i]!=ci->op_value[i]) {
          match = 0;
          break;
        }
      }

      /* Not match, continue search */
      if(!match) {
        continue;
      }

      /* Match: return id */
      return id;
    }
  }
  return ERROR_NOT_FOUND;
}

/*
 * Program entry point
 */
uint main(uint argc, uchar* argv[])
{
  uint i=0, j=0;
  uchar ofile[14];

  /* Input file buffer and offset */
  uchar fbuff[2048];
  uint  foffset = 0;
  uint  fline = 1;

  /* Output buffer and offset */
  uchar obuff[1024];
  uint  ooffset = 0;

  /* Check args */
  if(argc != 2) {
    putstr("usage: %s <inputfile>\n\r", argv[0]);
    return 0;
  }

  /* Compute output file name */
  strcpy_s(ofile, argv[1], sizeof(ofile));
  i = strchr(ofile, '.');
  if(i) {
    ofile[i-1] = 0;
  }
  ofile[sizeof(ofile)-strlen(".bin")-2] = 0;
  strcat_s(ofile, ".bin", sizeof(ofile));

  /* Clear output buffer */
  memset(obuff, 0, sizeof(obuff));

  /* Clear symbols table */
  memset(s_table, 0, sizeof(s_table));

  /* Clear references table */
  memset(s_ref, 0, sizeof(s_ref));

  /* Process input file line by line */
  while((foffset=read_line(fbuff, sizeof(fbuff), argv[1], foffset))!=EOF) {
    uint   tokc = 0;
    uchar* tokv[32];

    /* Exit on read error */
    if(foffset >= ERROR_ANY) {
      putstr("Error reading input file\n\r");
      debugstr("Error (%x) reading input file (%s)\n\r", foffset, argv[0]);
      ooffset = 0;
      break;
    }

    /* Tokenize line */
    tokc = tokenize_line(fbuff, sizeof(fbuff), tokv, sizeof(tokv)/sizeof(uchar*));

    /* Check special directives */
    if(tokc==2 && !strcmp(tokv[0], "ORG")) {
      cg_origin = stou(tokv[1]);
      debugstr("origin = %x\n\r", cg_origin);
    }

    /* Check labels */
    else if(tokc==1 && tokv[0][strlen(tokv[0])-1] == ':') {
      tokv[0][strlen(tokv[0])-1] = 0;
      j = find_or_add_symbol(tokv[0]);
      s_table[j].value = ooffset;
      s_table[j].type = S_LABEL;
      debugstr("label %s = ORG+%x\n\r", s_table[j].name, s_table[j].value);
    }

    /* Check memory addresses */
    else if(tokc>=3 && !strcmp(tokv[1], "dw")) {
      uint n = 0;
      j = find_or_add_symbol(tokv[0]);
      s_table[j].value = ooffset;
      s_table[j].type = S_DATAW;
      debugstr("word %s = ORG+%x : ", s_table[j].name, s_table[j].value);
      for(n=2; n<tokc; n++) {
        debugstr("%x ", stou(tokv[n]));
        ooffset=encode_data(obuff, ooffset, s_table[j].type, stou(tokv[n]));
      }
      debugstr("\n\r");
    }
    else if(tokc>=3 && !strcmp(tokv[1], "db")) {
      uint n = 0;
      j = find_or_add_symbol(tokv[0]);
      s_table[j].value = ooffset;
      s_table[j].type = S_DATAB;
      debugstr("byte %s = ORG+%x : ", s_table[j].name, s_table[j].value);
      for(n=2; n<tokc; n++) {
        debugstr("%2x ", stou(tokv[n]));
        ooffset=encode_data(obuff, ooffset, s_table[j].type, stou(tokv[n]));
      }
      debugstr("\n\r");
    }

    /* Check instructions */
    else if(tokc) {
      uint   s=0xFFFF, nas=0xFFFF;
      uchar  instr_id = 0;
      struct idata ci;

      /* Get instruction info */
      strcpy_s(ci.mnemonic, tokv[0], sizeof(ci.mnemonic));
      ci.opcode = 0;
      ci.nops = tokc-1;
      debugstr("%s%6c", ci.mnemonic, ' ');

      for(i=0; i<ci.nops; i++) {
        uint is_pointer = 0;
        ci.op_type[i] = O_ANY;
        ci.op_value[i] = 0;

        if(tokv[i+1][0]=='[' && tokv[i+1][strlen(tokv[i+1])-1]==']') {
          is_pointer = 1;
          tokv[i+1][strlen(tokv[i+1])-1] = 0;
          tokv[i+1]++;
        }

        /* Check registers */
        for(j=0; j<R_COUNT; j++) {
          if(!strcmp(tokv[i+1], r_data[j].name)) {
            ci.op_type[i] = is_pointer ? O_RMW : r_data[j].type;
            ci.op_value[i] = j;
            debugstr("%s:%s%9c", is_pointer?"pr":"r ",r_data[j].name, ' ');
            break;
          }
        }

        /* If not a register, check immediate */
        if(ci.op_type[i] == O_ANY && sisu(tokv[i+1])) {
          ci.op_type[i] = is_pointer ? O_MW : O_IW;
          ci.op_value[i] = stou(tokv[i+1]);
          debugstr("%s:%x%9c", is_pointer?"p":"i",ci.op_value[i], ' ');
        }

        /* Otherwise, it must be a symbol */
        if(ci.op_type[i] == O_ANY) {
          s = find_or_add_symbol(tokv[i+1]);
          nas = i;

          ci.op_type[i] = is_pointer ? O_MW : O_IW;
          ci.op_value[i] = stou(tokv[i+1]);
          debugstr("%s:%s%9c", is_pointer?"p":"i", tokv[i+1], ' ');
        }
      }

      /* Make debug info look better */
      for(j=i; j<I_MAX_OPS; j++) {
        debugstr("%9c", ' ');
      }

      /* Process line */
      instr_id = find_instruction(&ci);

      /* Show error if no opcode found */
      if(instr_id == ERROR_NOT_FOUND) {
        putstr("error: line %u. Instruction not found\n\r", fline);
        debugstr("error: line %u. Instruction not found\n\r", fline);
        ooffset = 0;
        break;
      }

      /* Set opcode */
      ci.opcode = i_data[instr_id].opcode;

      /* Append symbol reference to table */
      if(s != 0xFFFF) {
        append_symbol_ref(s, nas, ooffset, instr_id, &ci);
      }

      /* Write instruction to buffer */
      ooffset = encode_instruction(obuff, ooffset, instr_id, ci.op_value);

      /* Show error if not able to encode */
      if(ooffset == ERROR_NOT_FOUND) {
        putstr("error: line %u. Can't encode instruction\n\r", fline);
        debugstr("error: line %u. Can't encode instruction\n\r", fline);
        ooffset = 0;
        break;
      }

      debugstr("\n\r");
    }
    fline++;
  }

  /* Symbol table is filled, resolve references */
  if(ooffset != 0) {
    uint r = 0;
    for(r=0; r<S_MAX_REF; r++) {

      /* No more symbols: break */
      if(s_ref[r].offset == 0) {
        break;
      }

      /* Correct arg value */
      s_ref[r].instruction.op_value[s_ref[r].operand] =
        s_table[s_ref[r].symbol].value + cg_origin;

      debugstr("Solved symbol. Instruction %s%32c at %x : arg %d : %s%60c = %x ",
        i_data[s_ref[r].instr_id].mnemonic, ' ',
        s_ref[r].offset,
        s_ref[r].operand,
        s_table[s_ref[r].symbol].name, ' ',
        s_ref[r].instruction.op_value[s_ref[r].operand],
        ' ');

      /* Overwrite the old instruction */
      encode_instruction(obuff, s_ref[r].offset,
        s_ref[r].instr_id, s_ref[r].instruction.op_value);

      debugstr("\n\r");
    }
  }

  /* Write output file */
  if(ooffset != 0) {
    debugstr("Write file: %s, %ubytes\n\r", ofile, ooffset);
    write_file(obuff, ofile, 0, ooffset, FWF_CREATE|FWF_TRUNCATE);
    debugstr("Done\n\r\n\r", ofile, ooffset);

    /* Dump saved file */
    debugstr("Dump: \n\r", ofile, ooffset);

    read_file(obuff, ofile, 0, ooffset);
    for(i=0; i<ooffset; i++) {
      debugstr("%2x ", obuff[i]);
    }

    debugstr("\n\r\n\r");
  }

  return 0;
}
