#pragma once

class Player {
public:
  Player();
  ~Player();

  float x, y;           // Position
  float dirX, dirY;     // Direction vector
  float planeX, planeY; // Camera plane

  float moveSpeed;
  float rotSpeed;

  void update(float deltaTime);
};
