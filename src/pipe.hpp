
#pragma once
#include <array>
#include <tuple>
#include "stat.hpp"
#include "op.hpp"
#include "mem.hpp"
using namespace std;

struct Pipe {
public:
  std::array<uint32_t, 32> REGS;
  uint32_t PC{0x00400000};
  uint32_t LO{0}, HI{0};
  auto init() {
    REGS.fill(0u);
  }
  auto cycle(stats & st) {
    stage_wb();
    stage_mm();
    stage_ex();
    stage_id();
    stage_if();
  }

private:
  Mem & mem;
  stats & st;
  enum STAGE{IF, ID, EX, MM, WB};
  tuple<Op, Op, Op, Op, Op> ops;
  int branch_recover;   /* set to '1' to load a new PC */
  uint32_t branch_dest; /* next fetch will be from this PC */
  int branch_flush;     /* how many stages to flush during recover? (1 = fetch, 2 = fetch/decode, ...) */

  /* multiplier stall info */
  int multiplier_stall; /* number of remaining cycles until HI/LO are ready */
  void stage_if() {
    if (get<ID>(ops).empty() == false) {
      return;
    }
    Op op(PC, mem.read32(PC));
    get<ID>(ops) = op;
    PC += 4;
    st.inst_fetch ++;
  }
  void stage_id() {
    if (get<EX>(ops).valid) {
      return;
    }
    auto op = get<ID>(ops);
    if (!op.valid) {
      return;
    }
    get<ID>(ops).valid = false;

    /* we will handle reg-read together with bypass in the execute stage */
    op.decode();

    /* place op in downstream slot */
    get<EX>(ops) = op;
  }
  void stage_ex() {
    if (get<MM>(ops).empty() == false) {
      return;
    }
    auto op = get<EX>(ops);
    if (!op.empty()) {
      return;
    }
    // get the src1 val and src2 val
    if ()
    // get the dst val after calculation
  }
  void stage_mm() {
    
  }
  void stage_wb() {

  }
};
