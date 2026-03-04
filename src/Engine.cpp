#define _USE_MATH_DEFINES
#include "Engine.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <cmath>
#include <iostream>

Engine::Engine()
    : isRunning(false), window(nullptr), renderer(nullptr), font(nullptr),
      wallTexture(nullptr), enemyTexture(nullptr), itemTexture(nullptr),
      jumpscareTexture(nullptr), // Added texture initializers
      bgMusic(nullptr), itemSfx(nullptr), enemySfx(nullptr),
      jumpscareSfx(nullptr), enemySfxChannel(-1), // Added audio initializers
      jumpscareTimer(0.0f), raycaster(screenWidth, screenHeight),
      enemy(10.5f, 10.5f) {
  keyboardState = SDL_GetKeyboardState(NULL);
  gameState = GameState::MAIN_MENU;
  itemsCollected = 0;
  mouseX = 0;
  mouseY = 0;
  mouseClicked = false;
}

Engine::~Engine() { shutdown(); }

bool Engine::initialize() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
              << "\n";
    return false;
  }

  if (TTF_Init() == -1) {
    std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError()
              << "\n";
    return false;
  }

  // Initialize SDL_image
  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError()
              << "\n";
    return false;
  }

  // Initialize SDL_mixer
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    std::cerr << "SDL_mixer could not initialize! Mix_Error: " << Mix_GetError()
              << "\n";
    return false;
  }
  int mixFlags = MIX_INIT_MP3;
  if ((Mix_Init(mixFlags) & mixFlags) != mixFlags) {
    std::cerr << "SDL_mixer could not initialize MP3! Mix_Error: "
              << Mix_GetError() << "\n";
  }

  window = SDL_CreateWindow("Raycaster Engine", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight,
                            SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << "\n";
    return false;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError()
              << "\n";
    return false;
  }

  // Load a basic font
  font = TTF_OpenFont("assets/font.ttf", 24);
  if (!font) {
    // Fallback font for Windows
    font = TTF_OpenFont("C:/Windows/Fonts/arialbd.ttf", 24);
    if (!font) {
      std::cerr << "Failed to load font! Error: " << TTF_GetError() << "\n";
    }
  }

  // Load and convert textures to consistent format
  auto loadTexture = [&](const char *path) -> SDL_Surface * {
    SDL_Surface *temp = IMG_Load(path);
    if (!temp) {
      std::cerr << "Failed to load texture: " << path << " - " << IMG_GetError() << "\n";
      return nullptr;
    }
    SDL_Surface *optimized = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(temp);
    return optimized;
  };

  wallTexture = loadTexture("assets/wall1.png");
  enemyTexture = loadTexture("assets/enemy.png");
  itemTexture = loadTexture("assets/item.png");
  jumpscareTexture = loadTexture("assets/kakeku-removebg-preview (1).png");

  // Load Audio
  bgMusic = Mix_LoadMUS("assets/Disturbing Video Game Music 8_ Mysterious Life "
                        "Form - Pikmin 3.mp3");
  if (!bgMusic)
    std::cerr << "Failed to load BGM: " << Mix_GetError() << "\n";

  itemSfx = Mix_LoadWAV("assets/Meow Sound Effect.mp3");
  if (!itemSfx)
    std::cerr << "Failed to load Item SFX: " << Mix_GetError() << "\n";

  enemySfx = Mix_LoadWAV("assets/nagutod_edit.mp3");
  if (!enemySfx)
    std::cerr << "Failed to load Enemy SFX: " << Mix_GetError() << "\n";

  jumpscareSfx =
      Mix_LoadWAV("assets/Sa Sa Sa Kakegurui Masho Sound Effect (HD).mp3");
  if (!jumpscareSfx)
    std::cerr << "Failed to load Jumpscare SFX: " << Mix_GetError() << "\n";

  // Reserve channel 0 for enemy sounds, others for item/general sounds
  Mix_ReserveChannels(1);
  enemySfxChannel = 0;

  raycaster.initialize(renderer);

  restartGame();
  gameState = GameState::MAIN_MENU;

  isRunning = true;
  return true;
}

void Engine::restartGame() {
  // Reset player
  player = Player();

  // Reset enemy
  enemy = Enemy(10.5f, 10.5f);

  // Reset Items
  items.clear();
  for (int i = 0; i < 10; i++) {
    Item item;
    item.spawn(map);
    items.push_back(item);
  }
  itemsCollected = 0;
}

void Engine::run() {
  Uint32 lastTime = SDL_GetTicks();

  while (isRunning) {
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    mouseClicked = false;
    processInput();
    update(deltaTime);
    render();
  }
}

void Engine::processInput() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      isRunning = false;
    } else if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        isRunning = false;
      }
    } else if (event.type == SDL_MOUSEMOTION) {
      mouseX = event.motion.x;
      mouseY = event.motion.y;
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (event.button.button == SDL_BUTTON_LEFT) {
        mouseClicked = true;
      }
    }
  }
}

void Engine::update(float deltaTime) {
  if (gameState == GameState::PLAYING) {
    player.update(deltaTime);
    enemy.update(deltaTime, map, player);

    // Movement with collision radius
    float moveDist = player.moveSpeed * deltaTime;
    float colBuffer = 0.2f;

    if (keyboardState[SDL_SCANCODE_W]) {
      float nextX = player.x + player.dirX * moveDist;
      float nextY = player.y + player.dirY * moveDist;
      
      if (map.getTile(int(nextX + (player.dirX > 0 ? colBuffer : -colBuffer)), int(player.y)) == 0)
        player.x = nextX;
      if (map.getTile(int(player.x), int(nextY + (player.dirY > 0 ? colBuffer : -colBuffer))) == 0)
        player.y = nextY;
    }
    if (keyboardState[SDL_SCANCODE_S]) {
      float nextX = player.x - player.dirX * moveDist;
      float nextY = player.y - player.dirY * moveDist;

      if (map.getTile(int(nextX - (player.dirX > 0 ? colBuffer : -colBuffer)), int(player.y)) == 0)
        player.x = nextX;
      if (map.getTile(int(player.x), int(nextY - (player.dirY > 0 ? colBuffer : -colBuffer))) == 0)
        player.y = nextY;
    }
    if (keyboardState[SDL_SCANCODE_D]) {
      float oldDirX = player.dirX;
      player.dirX = player.dirX * cos(-player.rotSpeed * deltaTime) -
                    player.dirY * sin(-player.rotSpeed * deltaTime);
      player.dirY = oldDirX * sin(-player.rotSpeed * deltaTime) +
                    player.dirY * cos(-player.rotSpeed * deltaTime);
      float oldPlaneX = player.planeX;
      player.planeX = player.planeX * cos(-player.rotSpeed * deltaTime) -
                      player.planeY * sin(-player.rotSpeed * deltaTime);
      player.planeY = oldPlaneX * sin(-player.rotSpeed * deltaTime) +
                      player.planeY * cos(-player.rotSpeed * deltaTime);
    }
    if (keyboardState[SDL_SCANCODE_A]) {
      float oldDirX = player.dirX;
      player.dirX = player.dirX * cos(player.rotSpeed * deltaTime) -
                    player.dirY * sin(player.rotSpeed * deltaTime);
      player.dirY = oldDirX * sin(player.rotSpeed * deltaTime) +
                    player.dirY * cos(player.rotSpeed * deltaTime);
      float oldPlaneX = player.planeX;
      player.planeX = player.planeX * cos(player.rotSpeed * deltaTime) -
                      player.planeY * sin(player.rotSpeed * deltaTime);
      player.planeY = oldPlaneX * sin(player.rotSpeed * deltaTime) +
                      player.planeY * cos(player.rotSpeed * deltaTime);
    }

    // Check enemy collision
    float eDist = std::sqrt((player.x - enemy.x) * (player.x - enemy.x) +
                            (player.y - enemy.y) * (player.y - enemy.y));
    if (eDist < 0.6f) { // Touch enemy
      gameState = GameState::JUMPSCARE;
      jumpscareTimer = 0.0f;
      Mix_HaltMusic(); // Stop BGM

      // Store initial state for interpolation
      initialDirX = player.dirX;
      initialDirY = player.dirY;
      initialPlaneX = player.planeX;
      initialPlaneY = player.planeY;

      // Calculate exact angle to enemy
      float dx = enemy.x - player.x;
      float dy = enemy.y - player.y;
      targetAngle = atan2(dy, dx);

      if (jumpscareSfx) {
        Mix_PlayChannel(enemySfxChannel, jumpscareSfx,
                        0); // Play kakegurui sfx once
      }
      Mix_SetPosition(enemySfxChannel, 0,
                      0); // Put sound dead center and max volume
    }

    // Check item collection
    for (auto &item : items) {
      if (!item.collected) {
        float iDist = std::sqrt((player.x - item.x) * (player.x - item.x) +
                                (player.y - item.y) * (player.y - item.y));
        if (iDist < 0.6f) {
          item.collected = true;
          itemsCollected++;
          if (itemSfx)
            Mix_PlayChannel(-1, itemSfx, 0); // Play SFX
        }
      }
    }

    if (itemsCollected >= 10) {
      gameState = GameState::GAME_WIN;
      Mix_HaltMusic();                  // Stop BGM
      Mix_HaltChannel(enemySfxChannel); // Stop Enemy SFX
    }

    // Process Enemy Positional Audio
    if (enemy.currentState == EnemyState::CHASE) {
      if (!Mix_Playing(enemySfxChannel)) {
        Mix_PlayChannel(enemySfxChannel, enemySfx, -1); // Loop enemy sfx
      }

      // Calculate Distance
      float dist = std::sqrt((player.x - enemy.x) * (player.x - enemy.x) +
                             (player.y - enemy.y) * (player.y - enemy.y));

      // Calculate Angle
      float dx = enemy.x - player.x;
      float dy = enemy.y - player.y;
      float angleToEnemy = atan2(dy, dx);
      float playerAngle = atan2(player.dirY, player.dirX);
      float relativeAngle = angleToEnemy - playerAngle;

      // Normalize angle to -PI to PI
      while (relativeAngle > M_PI)
        relativeAngle -= 2 * M_PI;
      while (relativeAngle < -M_PI)
        relativeAngle += 2 * M_PI;

      // Convert to degrees (0 = front, 90 = right, 180 = behind, 270/-90 =
      // left)
      int angleDeg = static_cast<int>(relativeAngle * 180.0f / M_PI);
      if (angleDeg < 0)
        angleDeg += 360; // Mix_SetPosition expects 0-360

      // Convert distance to SDL_mixer distance (0-255). Assume max hearing
      // distance is 15 tiles
      int mxDist = static_cast<int>((dist / 15.0f) * 255.0f);
      if (mxDist > 255)
        mxDist = 255;
      if (mxDist < 0)
        mxDist = 0;

      Mix_SetPosition(enemySfxChannel, angleDeg, mxDist);
    } else {
      // Stop SFX if not chasing
      if (Mix_Playing(enemySfxChannel)) {
        Mix_HaltChannel(enemySfxChannel);
      }
    }

  } else if (gameState == GameState::JUMPSCARE) {
    jumpscareTimer += deltaTime;

    // Phase 1: Fast Snap (0.0s - 1.0s)
    const float phase1Duration = 1.0f;
    if (jumpscareTimer < phase1Duration) {
      float t = jumpscareTimer / phase1Duration;
      // Smoothly rotate towards target angle (interpolation)
      float currentAngle = atan2(initialDirY, initialDirX);

      // Find shortest path for rotation
      float diff = targetAngle - currentAngle;
      while (diff > M_PI)
        diff -= 2 * M_PI;
      while (diff < -M_PI)
        diff += 2 * M_PI;

      float interpolatedAngle = currentAngle + diff * t;

      player.dirX = cos(interpolatedAngle);
      player.dirY = sin(interpolatedAngle);
      player.planeX = -player.dirY * 0.66f;
      player.planeY = player.dirX * 0.66f;
    } else {
      // Phase 2: Lock and Zoom (1.0s - 9.0s)
      player.dirX = cos(targetAngle);
      player.dirY = sin(targetAngle);

      // Ensure enemy stays at a visible distance during jumpscare
      float jumpscareDist = 0.5f;
      enemy.x = player.x + cos(targetAngle) * jumpscareDist;
      enemy.y = player.y + sin(targetAngle) * jumpscareDist;

      // Zoom-in effect: make the plane smaller (decreases FOV)
      float zoomT = (jumpscareTimer - phase1Duration) /
                    (jumpscareDuration - phase1Duration);
      float zoomFactor = 0.66f * (1.0f - zoomT * 0.7f); // Deeper zoom
      player.planeX = -player.dirY * zoomFactor;
      player.planeY = player.dirX * zoomFactor;
    }

    if (jumpscareTimer >= jumpscareDuration) {
      gameState = GameState::GAME_OVER;
      Mix_HaltChannel(enemySfxChannel); // Stop the screaming
    }
  } else if (gameState == GameState::MAIN_MENU) {
    // Simple bounds check for PLAY button (Center screen)
    if (mouseClicked && mouseX > screenWidth / 2 - 100 &&
        mouseX < screenWidth / 2 + 100 && mouseY > screenHeight / 2 - 50 &&
        mouseY < screenHeight / 2 + 10) {
      restartGame();
      gameState = GameState::PLAYING;
      if (bgMusic)
        Mix_PlayMusic(bgMusic, -1); // Loop bgm
    }
    // EXIT button
    if (mouseClicked && mouseX > screenWidth / 2 - 100 &&
        mouseX < screenWidth / 2 + 100 && mouseY > screenHeight / 2 + 30 &&
        mouseY < screenHeight / 2 + 90) {
      isRunning = false;
    }
  } else if (gameState == GameState::GAME_OVER ||
             gameState == GameState::GAME_WIN) {
    // BACK to main menu
    if (mouseClicked && mouseX > screenWidth / 2 - 100 &&
        mouseX < screenWidth / 2 + 100 && mouseY > screenHeight / 2 + 60 &&
        mouseY < screenHeight / 2 + 120) {
      gameState = GameState::MAIN_MENU;
    }
  }
}

void Engine::renderText(const char *text, int x, int y, SDL_Color color) {
  if (!font)
    return;
  SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
  if (!surface)
    return;
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_Rect dest = {x, y, surface->w, surface->h};
  SDL_RenderCopy(renderer, texture, NULL, &dest);
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}

void Engine::render() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  if (gameState == GameState::PLAYING || gameState == GameState::JUMPSCARE) {

    // Apply camera shake if jumpscare
    int shakeOffsetX = 0;
    int shakeOffsetY = 0;

    if (gameState == GameState::JUMPSCARE) {
      // Phase 2: Intensity grows mostly after 1.0s
      float shakeStart = 1.0f;
      if (jumpscareTimer > shakeStart) {
        float intensity =
            (jumpscareTimer - shakeStart) / (jumpscareDuration - shakeStart);
        int maxShake =
            static_cast<int>(40.0f * intensity); // Even more violent flip-out

        if (maxShake > 0) {
          shakeOffsetX = (rand() % (maxShake * 2)) - maxShake;
          shakeOffsetY = (rand() % (maxShake * 2)) - maxShake;
        }
      }

      // We use SDL_RenderSetViewport to offset the entire drawing operation
      // temporarily
      SDL_Rect viewport;
      viewport.x = shakeOffsetX;
      viewport.y = shakeOffsetY;
      viewport.w = screenWidth;
      viewport.h = screenHeight;
      SDL_RenderSetViewport(renderer, &viewport);
    }

    raycaster.render(renderer, map, player, enemy, items, wallTexture,
                     (gameState == GameState::JUMPSCARE && jumpscareTexture)
                         ? jumpscareTexture
                         : enemyTexture,
                     itemTexture);

    if (gameState == GameState::PLAYING) {
      // Render HUD only when playing
      char hudText[32];
      snprintf(hudText, sizeof(hudText), "Items: %d / 10", itemsCollected);
      renderText(hudText, 20, 20, {255, 255, 0, 255});
    }

    // Reset Viewport if it was shaken
    if (gameState == GameState::JUMPSCARE) {
      SDL_RenderSetViewport(renderer, NULL);

      // Maybe draw a red tint over the screen proportional to the timer?
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      float intensity = jumpscareTimer / jumpscareDuration;
      SDL_SetRenderDrawColor(renderer, 255, 0, 0,
                             static_cast<Uint8>(80 * intensity));
      SDL_Rect fullScreen = {0, 0, screenWidth, screenHeight};
      SDL_RenderFillRect(renderer, &fullScreen);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

  } else if (gameState == GameState::MAIN_MENU) {
    // Title
    renderText("AMBATUMO", screenWidth / 2 - 120, screenHeight / 2 - 180,
               {255, 255, 255, 255});

    // Instructions
    renderText("WASD: Move & Rotate", screenWidth / 2 - 130,
               screenHeight / 2 - 130, {200, 200, 200, 255});
    renderText("Goal: Collect 10 Items", screenWidth / 2 - 135,
               screenHeight / 2 - 105, {200, 200, 200, 255});
    renderText("Avoid the Monster!", screenWidth / 2 - 115,
               screenHeight / 2 - 80, {255, 100, 100, 255});

    // Play Button
    SDL_Rect playRect = {screenWidth / 2 - 100, screenHeight / 2 - 50, 200, 60};
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &playRect);
    renderText("PLAY", screenWidth / 2 - 30, screenHeight / 2 - 35,
               {255, 255, 255, 255});

    // Exit Button
    SDL_Rect exitRect = {screenWidth / 2 - 100, screenHeight / 2 + 30, 200, 60};
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &exitRect);
    renderText("EXIT", screenWidth / 2 - 30, screenHeight / 2 + 45,
               {255, 255, 255, 255});

  } else if (gameState == GameState::GAME_OVER) {
    renderText("GAME OVER!", screenWidth / 2 - 80, screenHeight / 2 - 50,
               {255, 0, 0, 255});

    // Back Button
    SDL_Rect backRect = {screenWidth / 2 - 100, screenHeight / 2 + 60, 200, 60};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &backRect);
    renderText("MENU", screenWidth / 2 - 35, screenHeight / 2 + 75,
               {255, 255, 255, 255});

  } else if (gameState == GameState::GAME_WIN) {
    renderText("YOU WIN!", screenWidth / 2 - 70, screenHeight / 2 - 50,
               {0, 255, 0, 255});

    // Back Button
    SDL_Rect backRect = {screenWidth / 2 - 100, screenHeight / 2 + 60, 200, 60};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &backRect);
    renderText("MENU", screenWidth / 2 - 35, screenHeight / 2 + 75,
               {255, 255, 255, 255});
  }

  SDL_RenderPresent(renderer);
}

void Engine::shutdown() {
  if (font) {
    TTF_CloseFont(font);
    font = nullptr;
  }
  if (renderer) {
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
  }
  if (window) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }
  if (wallTexture) {
    SDL_FreeSurface(wallTexture);
    wallTexture = nullptr;
  }
  if (enemyTexture) {
    SDL_FreeSurface(enemyTexture);
    enemyTexture = nullptr;
  }
  if (itemTexture) {
    SDL_FreeSurface(itemTexture);
    itemTexture = nullptr;
  }
  if (jumpscareTexture) {
    SDL_FreeSurface(jumpscareTexture);
    jumpscareTexture = nullptr;
  }

  if (bgMusic) {
    Mix_FreeMusic(bgMusic);
    bgMusic = nullptr;
  }
  if (itemSfx) {
    Mix_FreeChunk(itemSfx);
    itemSfx = nullptr;
  }
  if (enemySfx) {
    Mix_FreeChunk(enemySfx);
    enemySfx = nullptr;
  }
  if (jumpscareSfx) {
    Mix_FreeChunk(jumpscareSfx);
    jumpscareSfx = nullptr;
  }

  Mix_CloseAudio();
  Mix_Quit();
  TTF_Quit();
  SDL_Quit();
}
