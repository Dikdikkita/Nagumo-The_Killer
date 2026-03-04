#include "Enemy.h"
#include <cmath>
#include <cstdlib>

Enemy::Enemy(float startX, float startY) : x(startX), y(startY) {
  moveSpeed = 2.0f;
  currentState = EnemyState::PATROL;
  targetX = x;
  targetY = y;
}

Enemy::~Enemy() {}

void Enemy::pickRandomPatrolTarget(const Map &map) {
  bool valid = false;
  while (!valid) {
    // Map is 24x24
    int randX = std::rand() % Map::mapWidth;
    int randY = std::rand() % Map::mapHeight;

    if (map.getTile(randX, randY) == 0) {
      targetX = randX + 0.5f;
      targetY = randY + 0.5f;
      valid = true;
    }
  }
}

bool Enemy::hasLineOfSight(const Map &map, const Player &player) {
  float rayDirX = player.x - x;
  float rayDirY = player.y - y;
  float dist = std::sqrt(rayDirX * rayDirX + rayDirY * rayDirY);

  rayDirX /= dist;
  rayDirY /= dist;

  int mapX = int(x);
  int mapY = int(y);

  float deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
  float deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);

  float sideDistX;
  float sideDistY;

  int stepX;
  int stepY;

  if (rayDirX < 0) {
    stepX = -1;
    sideDistX = (x - mapX) * deltaDistX;
  } else {
    stepX = 1;
    sideDistX = (mapX + 1.0f - x) * deltaDistX;
  }
  if (rayDirY < 0) {
    stepY = -1;
    sideDistY = (y - mapY) * deltaDistY;
  } else {
    stepY = 1;
    sideDistY = (mapY + 1.0f - y) * deltaDistY;
  }

  bool hit = false;
  float currentDist = 0.0f;

  while (!hit && currentDist < dist) {
    if (sideDistX < sideDistY) {
      sideDistX += deltaDistX;
      mapX += stepX;
      currentDist = sideDistX - deltaDistX;
    } else {
      sideDistY += deltaDistY;
      mapY += stepY;
      currentDist = sideDistY - deltaDistY;
    }

    if (mapX == int(player.x) && mapY == int(player.y)) {
      return true;
    }

    if (map.getTile(mapX, mapY) > 0) {
      hit = true;
    }
  }

  return hit == false;
}

void Enemy::update(float deltaTime, const Map &map, const Player &player) {
  bool canSeePlayer = hasLineOfSight(map, player);

  if (canSeePlayer) {
    currentState = EnemyState::CHASE;
    targetX = player.x;
    targetY = player.y;
  } else if (currentState == EnemyState::CHASE) {
    // Lost sight of player, go investigate last known location
    currentState = EnemyState::INVESTIGATE;
  }

  float dirX = targetX - x;
  float dirY = targetY - y;
  float distToTarget = std::sqrt(dirX * dirX + dirY * dirY);

  if (distToTarget > 0.5f) {
    dirX /= distToTarget;
    dirY /= distToTarget;

    float newX = x + dirX * moveSpeed * deltaTime;
    float newY = y + dirY * moveSpeed * deltaTime;

    bool moved = false;
    if (map.getTile(int(newX), int(y)) == 0) {
      x = newX;
      moved = true;
    }
    if (map.getTile(int(x), int(newY)) == 0) {
      y = newY;
      moved = true;
    }

    if (!moved) {
      // Enemy is completely stuck against a corner or wall trying to reach the
      // target.
      if (currentState == EnemyState::PATROL ||
          currentState == EnemyState::INVESTIGATE) {
        // Pick a new target if we can't move towards the current one
        currentState = EnemyState::PATROL;
        pickRandomPatrolTarget(map);
      }
    }
  } else {
    // Reached target
    if (currentState == EnemyState::INVESTIGATE) {
      // Reached last known location, didn't find player
      currentState = EnemyState::PATROL;
      pickRandomPatrolTarget(map);
    } else if (currentState == EnemyState::PATROL) {
      // Reached patrol point, pick another one
      pickRandomPatrolTarget(map);
    } else if (currentState == EnemyState::CHASE) {
      // Very close to player, do nothing (attack range)
    }
  }

  // First time init for patrol
  if (currentState == EnemyState::PATROL && targetX == x && targetY == y) {
    pickRandomPatrolTarget(map);
  }
}
