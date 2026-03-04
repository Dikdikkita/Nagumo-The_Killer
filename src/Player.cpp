#include "Player.h"

Player::Player() {
  x = 22.0f;
  y = 12.0f;
  dirX = -1.0f;
  dirY = 0.0f;
  planeX = 0.0f;
  planeY = 0.66f;

  moveSpeed = 5.0f;
  rotSpeed = 3.0f;
}

Player::~Player() {}

void Player::update(float deltaTime) {
  // Basic update logic, movement will be handled via Engine input injecting or
  // direct state read
}
