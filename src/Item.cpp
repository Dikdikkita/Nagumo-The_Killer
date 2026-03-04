#include "Item.h"
#include <cstdlib>

Item::Item() : x(0), y(0), collected(false) {}

Item::~Item() {}

void Item::spawn(const Map &map) {
  bool valid = false;
  while (!valid) {
    int randX = std::rand() % Map::mapWidth;
    int randY = std::rand() % Map::mapHeight;

    if (map.getTile(randX, randY) == 0) {
      x = randX + 0.5f;
      y = randY + 0.5f;
      collected = false;
      valid = true;
    }
  }
}
