#pragma once

class Map {
public:
  Map();
  ~Map();

  // Map properties
  static const int mapWidth = 24;
  static const int mapHeight = 24;

  int getTile(int x, int y) const;
  void load();

private:
  int level[mapWidth][mapHeight];
};
