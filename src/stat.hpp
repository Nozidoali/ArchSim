#pragma once

struct stats {
  int cycle{0};
  int inst_fetch{0};
  int inst_retire{0};
  int flush{0};
  bool end{false};
};
