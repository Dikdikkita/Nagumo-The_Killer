#pragma once
#include "Map.h"
#include "Player.h"

enum class EnemyState { PATROL, CHASE, INVESTIGATE };

class Enemy {
public:
  Enemy(float startX, float startY);
  ~Enemy();

  float x;
  float y;
  float moveSpeed;

  EnemyState currentState;
  float targetX;
  float targetY;

  bool hasLineOfSight(const Map &map, const Player &player);
  void update(float deltaTime, const Map &map, const Player &player);
  void pickRandomPatrolTarget(const Map &map);
};
