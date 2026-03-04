#pragma once
#include "Enemy.h"
#include "Item.h"
#include "Map.h"
#include "Player.h"
#include <SDL.h>
#include <vector>

class Raycaster {
public:
  Raycaster(int width, int height);
  ~Raycaster();

  void initialize(SDL_Renderer *renderer);
  void render(SDL_Renderer *renderer, const Map &map, const Player &player,
              const Enemy &enemy, const std::vector<Item> &items,
              SDL_Surface *wallTexture, SDL_Surface *enemyTexture,
              SDL_Surface *itemTexture);

private:
  int screenWidth;
  int screenHeight;
  std::vector<float> zBuffer;
  SDL_Texture *screenTexture;
  std::vector<Uint32> buffer;
};
