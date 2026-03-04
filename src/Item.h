#pragma once
#include "Map.h"

class Item {
public:
  Item();
  ~Item();

  float x;
  float y;
  bool collected;

  void spawn(const Map &map);
};
