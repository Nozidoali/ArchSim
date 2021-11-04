/*!
  \file sim.hpp
  \brief Simulation Top Module
  \author Hanyu Wang
*/

#pragma once

#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include "pipe.hpp"
#include "mem.hpp"
#include "stat.hpp"

using namespace std;

struct Simulator {

public:
  auto parse(int argc, char ** argv) {
    for (int i=1;i<argc;i++) {
      load_program(argv[i]);
    }
  }
  auto get_command() {
    char buffer[20];
    int x, y;
    int cache_id;
    printf("MIPS-SIM> ");
    if (scanf("%s", buffer) == EOF)
      exit(0);
    printf("\n");
    switch(buffer[0]) {
    case 'G':
    case 'g':
      if (st.end) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
      }
      printf("Simulating...\n\n");
      while (!st.end)
        cycle();
      printf("Simulator halted\n\n");
      break;
    case 'M':
    case 'm':
      if (scanf("%i %i", &x, &y) != 2)
          break;
      printf("\nMemory content [0x%08x..0x%08x] :\n", x, y);
      printf("-------------------------------------\n");
      for (int i=x; i<=y; i+=4)
        printf("  0x%08x (%d) : 0x%08x\n", i, i, mem.read32(i));
      printf("\n");
      break;
    case '?':
      help();
      break;
    case 'Q':
    case 'q':
      printf("Bye.\n");
      exit(0);
    case 'R':
    case 'r':
      if (buffer[1] == 'd' || buffer[1] == 'D'){
        printf("PC: 0x%08x\n", pipe.PC);
        for (int i=0;i<32;i++) {
          printf("R%d: 0x%08x\n", i, pipe.REGS[i]);
        }
        printf("HI: 0x%08x\n", pipe.HI);
        printf("LO: 0x%08x\n", pipe.LO);
        printf("Cycles: %u\n", st.cycle);
        printf("FetchedInstr: %u\n", st.inst_fetch);
        printf("RetiredInstr: %u\n", st.inst_retire);
        printf("IPC: %0.3f\n", ((float) st.inst_retire) / st.cycle);
        printf("flush: %u\n", st.squash);
      }
      else {
        if (scanf("%d", &x) != 1) break;
          if (st.end) {
            printf("Can't simulate, Simulator is halted\n\n");
            return;
          }
          printf("Simulating for %d cycles...\n\n", x);
          for (int i=0; i<x; i++) {
            if (st.end) {
              printf("Simulator halted\n\n");
              break;
            }
            cycle();
          }
      }
      break;
    case 'I':
    case 'i':
      if (scanf("%i %i", &x, &y) != 2)
        break;
      printf("%i %i\n", x, y);
      pipe.REGS[x] = y;
      break;
    case 'H':
    case 'h':
      if (scanf("%i", &x) != 1)
        break;
      pipe.HI = x; 
      break;
    case 'L':
    case 'l':
      if (scanf("%i", &x) != 1)
        break;
      pipe.LO = x; 
      break;
    default:
      printf("Invalid Command\n");
      break;
    }
  }
private:
  stats st;
  Mem & mem;
  Pipe & pipe;
  auto load_program(char * f) {
    uint32_t x, addr = mem.TEXT;
    ifstream fin(f);
    if (!fin) {
      throw "Error: Can't open program file " + string(f);
    }
    while(fin >> hex >> x) {
      mem.write32(addr, x);
      addr+=4u;
    }
  }
  auto cycle() {
    pipe.cycle(st);
    st.cycle++;
  }
  auto help() {
    printf("----------------MIPS ISIM Help-----------------------\n");
    printf("go                     -  run program to completion         \n");
    printf("run n                  -  execute program for n instructions\n");
    printf("rdump                  -  dump architectural registers      \n");
    printf("mdump low high         -  dump memory from low to high      \n");
    printf("input reg_no reg_value - set GPR reg_no to reg_value  \n");
    printf("?                      -  display this help menu            \n");
    printf("quit                   -  exit the program                  \n\n");
  }
};

