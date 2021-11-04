
#pragma once
#include <array>
#include <iostream>
#include <cassert>
using namespace std;

struct Mem {
public:
  static const uint32_t TEXT = 0x00400000; /* Instruction */
  static const uint32_t DATA = 0x10000000; /* Data */
  auto init() {
    dram.fill(0u);
  }
  /* for virtual memory */
  auto map(uint32_t x) {
    if (x >= DATA) {
      return x-DATA + size;
    }
    if (x >= TEXT) {
      return x-TEXT;
    }
    return x;
  }
  auto read32(uint32_t addr) {
    auto addr_phys = map(addr);
    return
      (dram[addr_phys+3] << 0x18) |
      (dram[addr_phys+2] << 0x10) |
      (dram[addr_phys+1] << 0x08) |
      (dram[addr_phys+0] << 0x00);
  }
  auto write32(uint32_t addr, uint32_t val) {
    auto addr_phys = map(addr);
    dram[addr_phys+3] = (val >> 0x18) & 0xFF;
    dram[addr_phys+2] = (val >> 0x10) & 0xFF;
    dram[addr_phys+1] = (val >> 0x08) & 0xFF;
    dram[addr_phys+0] = (val >> 0x00) & 0xFF;
  }

private:
  static const uint32_t size = 0x00100000 * 2;
  std::array<uint8_t, size> dram; /* Byte addressable memory */

};
