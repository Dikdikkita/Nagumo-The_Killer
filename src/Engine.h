#pragma once
#include "Enemy.h"
#include "Item.h"
#include "Map.h"
#include "Player.h"
#include "Raycaster.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <vector>

enum class GameState { MAIN_MENU, PLAYING, JUMPSCARE, GAME_OVER, GAME_WIN };

class Engine {
public:
  Engine();
  ~Engine();

  bool initialize();
  void run();
  void shutdown();

private:
  void processInput();
  void update(float deltaTime);
  void render();
  void restartGame();
  void renderText(const char *text, int x, int y, SDL_Color color);

  bool isRunning;
  TTF_Font *font;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Surface *wallTexture;
  SDL_Surface *enemyTexture;
  SDL_Surface *itemTexture;
  SDL_Surface *jumpscareTexture;

  Mix_Music *bgMusic;
  Mix_Chunk *itemSfx;
  Mix_Chunk *enemySfx;
  Mix_Chunk *jumpscareSfx;
  int enemySfxChannel;

  const int screenWidth = 800;
  const int screenHeight = 600;

  GameState gameState;
  Map map;
  Player player;
  Enemy enemy;
  std::vector<Item> items;
  int itemsCollected;

  float jumpscareTimer;
  const float jumpscareDuration = 9.0f;
  float targetAngle;
  float initialDirX, initialDirY;
  float initialPlaneX, initialPlaneY;

  Raycaster raycaster;

  const Uint8 *keyboardState;
  int mouseX, mouseY;
  bool mouseClicked;
};
