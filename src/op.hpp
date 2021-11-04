#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cassert>
#include <tuple>
using namespace std;


/* special sub-opcodes (opcode=0) */
#define OP_SPECIAL 0x00
#define SUBOP_SLL 0x0
#define SUBOP_SRL 0x2
#define SUBOP_SRA 0x3
#define SUBOP_SLLV 0x4
#define SUBOP_SRLV 0x6
#define SUBOP_SRAV 0x7
#define SUBOP_JR   0x8
#define SUBOP_JALR 0x9
#define SUBOP_SYSCALL 0xc
#define SUBOP_MULT 0x18
#define SUBOP_MULTU 0x19
#define SUBOP_DIV  0x1a
#define SUBOP_DIVU 0x1b
#define SUBOP_ADD  0x20
#define SUBOP_ADDU 0x21
#define SUBOP_SUB  0x22
#define SUBOP_SUBU 0x23
#define SUBOP_AND  0x24
#define SUBOP_OR   0x25
#define SUBOP_XOR  0x26 
#define SUBOP_NOR  0x27
#define SUBOP_SLT  0x2A
#define SUBOP_SLTU 0x2B

#define SUBOP_MFHI 0x10
#define SUBOP_MTHI 0x11
#define SUBOP_MFLO 0x12
#define SUBOP_MTLO 0x13

/* special sub-opcodes (opcode=1) */
#define OP_BRSPEC 0x01 /* special branches */
#define BROP_BLTZ 0x00
#define BROP_BGEZ 0x1
#define BROP_BLTZAL 0x10
#define BROP_BGEZAL 0x11

/* primary opcodes */
#define OP_J     0x02
#define OP_JAL   0x03
#define OP_BEQ   0x04
#define OP_BNE   0x05
#define OP_BLEZ  0x06
#define OP_BGTZ  0x07
#define OP_ADDI  0x08
#define OP_ADDIU 0x09
#define OP_SLTI  0x0a
#define OP_SLTIU 0x0b
#define OP_ANDI  0x0c
#define OP_ORI   0x0d
#define OP_XORI  0x0e
#define OP_LUI   0x0f
#define OP_LB    0x20
#define OP_LH    0x21
#define OP_LW    0x23
#define OP_LBU   0x24
#define OP_LHU   0x25
#define OP_SB    0x28
#define OP_SH    0x29
#define OP_SW    0x2b

struct Op {
public:
  Op(uint32_t _pc, uint32_t _ins): pc(_pc), ins(_ins), valid(true){}
  auto retire() {
    assert(valid);
    valid = false;
  }
  auto empty() {
    return !valid;
  }
  auto get_reg_src() {
    return tuple<int, int>(reg_src1, reg_src2);
  }
  auto decode() {
    uint32_t opcode = (ins >> 26) & 0x3F;
    uint32_t rs =     (ins >> 21) & 0x1F;
    uint32_t rt =     (ins >> 16) & 0x1F;
    uint32_t rd =     (ins >> 11) & 0x1F;
    uint32_t shamt =  (ins >>  6) & 0x1F;
    uint32_t funct1 = (ins >>  0) & 0x1F;
    uint32_t funct2 = (ins >>  0) & 0x3F;
    uint32_t imm16 =  (ins >>  0) & 0xFFFF;
    uint32_t se_imm16 = imm16 | ((imm16 & 0x8000) ? 0xFFFF8000 : 0);
    uint32_t targ =   (ins & ((1UL << 26) - 1)) << 2;
    switch (opcode) {
      case OP_SPECIAL:
          /** all "SPECIAL" insts are R-types that use the ALU and both source
           *  regs. Set up source regs and immediate value. */
          reg_src1 = rs;
          reg_src2 = rt;
          reg_dst = rd;
          subop = funct2;
          if (funct2 == SUBOP_SYSCALL) {
              reg_src1 = 2; // v0
              reg_src2 = 3; // v1
          }
          if (funct2 == SUBOP_JR || funct2 == SUBOP_JALR) {
              is_branch = 1;
              branch_cond = 0;
          }
          break;
      case OP_BRSPEC:
          /* branches that have -and-link variants come here */
          is_branch = 1;
          reg_src1 = rs;
          reg_src2 = rt;
          is_branch = 1;
          branch_cond = 1; /* conditional branch */
          branch_dest = pc + 4 + (se_imm16 << 2);
          subop = rt;
          if (rt == BROP_BLTZAL || rt == BROP_BGEZAL) {
              /* link reg */
              reg_dst = 31;
              reg_dst_value = pc + 4;
              reg_dst_value_ready = 1;
          }
          break;

      case OP_JAL:
          reg_dst = 31;
          reg_dst_value = pc + 4;
          reg_dst_value_ready = 1;
          branch_taken = 1;
          /* fallthrough */
      case OP_J:
          is_branch = 1;
          branch_cond = 0;
          branch_taken = 1;
          branch_dest = (pc & 0xF0000000) | targ;
          break;

      case OP_BEQ:
      case OP_BNE:
      case OP_BLEZ:
      case OP_BGTZ:
          /* ordinary conditional branches (resolved after execute) */
          is_branch = 1;
          branch_cond = 1;
          branch_dest = pc + 4 + (se_imm16 << 2);
          reg_src1 = rs;
          reg_src2 = rt;
          break;

      case OP_ADDI:
      case OP_ADDIU:
      case OP_SLTI:
      case OP_SLTIU:
          /* I-type ALU ops with sign-extended immediates */
          reg_src1 = rs;
          reg_dst = rt;
          break;

      case OP_ANDI:
      case OP_ORI:
      case OP_XORI:
      case OP_LUI:
          /* I-type ALU ops with non-sign-extended immediates */
          reg_src1 = rs;
          reg_dst = rt;
          break;

      case OP_LW:
      case OP_LH:
      case OP_LHU:
      case OP_LB:
      case OP_LBU:
      case OP_SW:
      case OP_SH:
      case OP_SB:
          /* memory ops */
          is_mem = 1;
          reg_src1 = rs;
          if (opcode == OP_LW || opcode == OP_LH || opcode == OP_LHU || opcode == OP_LB || opcode == OP_LBU) {
              /* load */
              mem_write = 0;
              reg_dst = rt;
          }
          else {
              /* store */
              mem_write = 1;
              reg_src2 = rt;
          }
          break;
    }
  }
  auto execute() {
    uint32_t multiplier_stall{0}, HI{0}, LO{0};
    switch (opcode) {
    case OP_SPECIAL:
      reg_dst_value_ready = 1;
      switch (subop) {
        case SUBOP_SLL:
          reg_dst_value = reg_src2_value << shamt;
          break;
        case SUBOP_SLLV:
          reg_dst_value = reg_src2_value << reg_src1_value;
          break;
        case SUBOP_SRL:
          reg_dst_value = reg_src2_value >> shamt;
          break;
        case SUBOP_SRLV:
          reg_dst_value = reg_src2_value >> reg_src1_value;
          break;
        case SUBOP_SRA:
          reg_dst_value = (int32_t)reg_src2_value >> shamt;
          break;
        case SUBOP_SRAV:
          reg_dst_value = (int32_t)reg_src2_value >> reg_src1_value;
          break;
        case SUBOP_JR:
        case SUBOP_JALR:
          reg_dst_value = pc + 4;
          branch_dest = reg_src1_value;
          branch_taken = 1;
          break;

        case SUBOP_MULT:
          {
              /* we set a result value right away; however, we will
                * model a stall if the program tries to read the value
                * before it's ready (or overwrite HI/LO). Also, if
                * another multiply comes down the pipe later, it will
                * update the values and re-set the stall cycle count
                * for a new operation.
                */
              int64_t val = (int64_t)((int32_t)reg_src1_value) * (int64_t)((int32_t)reg_src2_value);
              uint64_t uval = (uint64_t)val;
              HI = (uval >> 32) & 0xFFFFFFFF;
              LO = (uval >>  0) & 0xFFFFFFFF;

              /* four-cycle multiplier latency */
              multiplier_stall = 4;
          }
          break;
        case SUBOP_MULTU:
          {
              uint64_t val = (uint64_t)reg_src1_value * (uint64_t)reg_src2_value;
              HI = (val >> 32) & 0xFFFFFFFF;
              LO = (val >>  0) & 0xFFFFFFFF;

              /* four-cycle multiplier latency */
              multiplier_stall = 4;
          }
          break;

        case SUBOP_DIV:
          if (reg_src2_value != 0) {

              int32_t val1 = (int32_t)reg_src1_value;
              int32_t val2 = (int32_t)reg_src2_value;
              int32_t div, mod;

              div = val1 / val2;
              mod = val1 % val2;

              LO = div;
              HI = mod;
          } else {
              // really this would be a div-by-0 exception
              HI = LO = 0;
          }

          /* 32-cycle divider latency */
          multiplier_stall = 32;
          break;

        case SUBOP_DIVU:
          if (reg_src2_value != 0) {
            HI = (uint32_t)reg_src1_value % (uint32_t)reg_src2_value;
            LO = (uint32_t)reg_src1_value / (uint32_t)reg_src2_value;
          } else {
            /* really this would be a div-by-0 exception */
            HI = LO = 0;
          }

          /* 32-cycle divider latency */
          multiplier_stall = 32;
          break;

        case SUBOP_MFHI:
          /* stall until value is ready */
          if (multiplier_stall > 0)
              return;

          reg_dst_value = HI;
          break;
        case SUBOP_MTHI:
          /* stall to respect WAW dependence */
          if (multiplier_stall > 0)
              return;

          HI = reg_src1_value;
          break;

        case SUBOP_MFLO:
          /* stall until value is ready */
          if (multiplier_stall > 0)
              return;

          reg_dst_value = LO;
          break;
        case SUBOP_MTLO:
          /* stall to respect WAW dependence */
          if (multiplier_stall > 0)
              return;

          LO = reg_src1_value;
          break;

        case SUBOP_ADD:
        case SUBOP_ADDU:
          reg_dst_value = reg_src1_value + reg_src2_value;
          break;
        case SUBOP_SUB:
        case SUBOP_SUBU:
          reg_dst_value = reg_src1_value - reg_src2_value;
          break;
        case SUBOP_AND:
          reg_dst_value = reg_src1_value & reg_src2_value;
          break;
        case SUBOP_OR:
          reg_dst_value = reg_src1_value | reg_src2_value;
          break;
        case SUBOP_NOR:
          reg_dst_value = ~(reg_src1_value | reg_src2_value);
          break;
        case SUBOP_XOR:
          reg_dst_value = reg_src1_value ^ reg_src2_value;
          break;
        case SUBOP_SLT:
          reg_dst_value = ((int32_t)reg_src1_value <
                  (int32_t)reg_src2_value) ? 1 : 0;
          break;
        case SUBOP_SLTU:
          reg_dst_value = (reg_src1_value < reg_src2_value) ? 1 : 0;
          break;
      }
      break;

    case OP_BRSPEC:
      switch (subop) {
          case BROP_BLTZ:
          case BROP_BLTZAL:
              if ((int32_t)reg_src1_value < 0) branch_taken = 1;
              break;

          case BROP_BGEZ:
          case BROP_BGEZAL:
              if ((int32_t)reg_src1_value >= 0) branch_taken = 1;
              break;
      }
      break;

    case OP_BEQ:
      if (reg_src1_value == reg_src2_value) branch_taken = 1;
      break;

    case OP_BNE:
      if (reg_src1_value != reg_src2_value) branch_taken = 1;
      break;

    case OP_BLEZ:
      if ((int32_t)reg_src1_value <= 0) branch_taken = 1;
      break;

    case OP_BGTZ:
      if ((int32_t)reg_src1_value > 0) branch_taken = 1;
      break;

    case OP_ADDI:
    case OP_ADDIU:
      reg_dst_value_ready = 1;
      reg_dst_value = reg_src1_value + se_imm16;
      break;
    case OP_SLTI:
      reg_dst_value_ready = 1;
      reg_dst_value = (int32_t)reg_src1_value < (int32_t)se_imm16 ? 1 : 0;
      break;
    case OP_SLTIU:
      reg_dst_value_ready = 1;
      reg_dst_value = (uint32_t)reg_src1_value < (uint32_t)se_imm16 ? 1 : 0;
      break;
    case OP_ANDI:
      reg_dst_value_ready = 1;
      reg_dst_value = reg_src1_value & imm16;
      break;
    case OP_ORI:
      reg_dst_value_ready = 1;
      reg_dst_value = reg_src1_value | imm16;
      break;
    case OP_XORI:
      reg_dst_value_ready = 1;
      reg_dst_value = reg_src1_value ^ imm16;
      break;
    case OP_LUI:
      reg_dst_value_ready = 1;
      reg_dst_value = imm16 << 16;
      break;

    case OP_LW:
    case OP_LH:
    case OP_LHU:
    case OP_LB:
    case OP_LBU:
      mem_addr = reg_src1_value + se_imm16;
      break;

    case OP_SW:
    case OP_SH:
    case OP_SB:
      mem_addr = reg_src1_value + se_imm16;
      mem_value = reg_src2_value;
      break;
    }
  }
private:
  bool valid;
  uint32_t pc;
  uint32_t ins;
  int opcode, subop;

  /* immediate value, if any, for ALU immediates */
  uint32_t imm16, se_imm16;
  /* shift amount */
  int shamt;

  /* register source values */
  int reg_src1, reg_src2; /* 0 -- 31 if this inst has register source(s), or
                              -1 otherwise */
  uint32_t reg_src1_value, reg_src2_value; /* values of operands from source
                                              regs */

  /* memory access information */
  int      is_mem;       /* is this a load/store? */
  uint32_t mem_addr;     /* address if applicable */
  int      mem_write;    /* is this a write to memory? */
  uint32_t mem_value;    /* value loaded from memory or to be written to memory */

  /* register destination information */
  int reg_dst; /* 0 -- 31 if this inst has a destination register, -1
                  otherwise */
  uint32_t reg_dst_value; /* value to write into dest reg. */
  int reg_dst_value_ready; /* destination value produced yet? */

  /* branch information */
  int is_branch;        /* is this a branch? */
  uint32_t branch_dest; /* branch destination (if taken) */
  int branch_cond;      /* is this a conditional branch? */
  int branch_taken;     /* branch taken? (set as soon as resolved: in decode
                            for unconditional, execute for conditional) */
  int is_link;          /* jump-and-link or branch-and-link inst? */
  int link_reg;         /* register to place link into? */
};

