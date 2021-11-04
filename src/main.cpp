#include "sim.hpp"

using basline = int;

int main(int argc, char *argv[]) {
  try {
    /* Error Checking */
    if (argc < 2) {
      throw "Error: usage: " + string(argv[0]) + " <program_file_1> <program_file_2> ...";
    }
    cout << "MIPS Simulator\n\n";
    auto sim = Sim();
    sim.parse(argc, argv);
    while (true) {
      sim.get_command();
    }
  } catch (string e) {
    cout << e << endl;
    exit(-1);
  }
}
