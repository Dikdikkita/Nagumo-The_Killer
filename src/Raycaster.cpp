#include "Raycaster.h"
#include <cmath>

Raycaster::Raycaster(int width, int height)
    : screenWidth(width), screenHeight(height), screenTexture(nullptr) {
  zBuffer.resize(screenWidth);
  buffer.resize(screenWidth * screenHeight);
}

Raycaster::~Raycaster() {
  if (screenTexture) {
    SDL_DestroyTexture(screenTexture);
  }
}

void Raycaster::initialize(SDL_Renderer *renderer) {
  screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING, screenWidth,
                                    screenHeight);
}

void Raycaster::render(SDL_Renderer *renderer, const Map &map,
                       const Player &player, const Enemy &enemy,
                       const std::vector<Item> &items, SDL_Surface *wallTexture,
                       SDL_Surface *enemyTexture, SDL_Surface *itemTexture) {
  // Clear buffer
  std::fill(buffer.begin(), buffer.end(), 0);

  // Ceiling and Floor colors (ARGB)
  Uint32 ceilingColor = 0xFF323232;
  Uint32 floorColor = 0xFF646464;

  for (int i = 0; i < screenWidth * screenHeight / 2; i++)
    buffer[i] = ceilingColor;
  for (int i = screenWidth * screenHeight / 2; i < screenWidth * screenHeight;
       i++)
    buffer[i] = floorColor;

  // Determine wall texture dimensions
  int texWidth = wallTexture ? wallTexture->w : 64;
  int texHeight = wallTexture ? wallTexture->h : 64;

  // Wall rendering
  for (int x = 0; x < screenWidth; x++) {
    // Calculate ray position and direction
    float cameraX =
        2 * x / (float)screenWidth - 1; // x-coordinate in camera space
    float rayDirX = player.dirX + player.planeX * cameraX;
    float rayDirY = player.dirY + player.planeY * cameraX;

    // Which box of the map we're in
    int mapX = int(player.x);
    int mapY = int(player.y);

    // Length of ray from current position to next x or y-side
    float sideDistX;
    float sideDistY;

    // Length of ray from one x or y-side to next x or y-side
    float deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
    float deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);
    float perpWallDist;

    // What direction to step in x or y-direction (either +1 or -1)
    int stepX;
    int stepY;

    int hit = 0; // Was there a wall hit?
    int side;    // Was a NS or a EW wall hit?

    // Calculate step and initial sideDist
    if (rayDirX < 0) {
      stepX = -1;
      sideDistX = (player.x - mapX) * deltaDistX;
    } else {
      stepX = 1;
      sideDistX = (mapX + 1.0f - player.x) * deltaDistX;
    }
    if (rayDirY < 0) {
      stepY = -1;
      sideDistY = (player.y - mapY) * deltaDistY;
    } else {
      stepY = 1;
      sideDistY = (mapY + 1.0f - player.y) * deltaDistY;
    }

    // Perform DDA
    while (hit == 0) {
      // Jump to next map square, either in x-direction, or in y-direction
      if (sideDistX < sideDistY) {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = 0;
      } else {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = 1;
      }
      // Check if ray has hit a wall
      if (map.getTile(mapX, mapY) > 0)
        hit = 1;
    }

    // Calculate distance projected on camera direction
    if (side == 0)
      perpWallDist = (sideDistX - deltaDistX);
    else
      perpWallDist = (sideDistY - deltaDistY);

    // Calculate height of line to draw on screen
    int lineHeight = (int)(screenHeight / perpWallDist);

    // Calculate lowest and highest pixel to fill in current stripe
    int drawStart = -lineHeight / 2 + screenHeight / 2;
    if (drawStart < 0)
      drawStart = 0;
    int drawEnd = lineHeight / 2 + screenHeight / 2;
    if (drawEnd >= screenHeight)
      drawEnd = screenHeight - 1;

    // Calculate exact hit coordinate for texture x
    float wallX;
    if (side == 0)
      wallX = player.y + perpWallDist * rayDirY;
    else
      wallX = player.x + perpWallDist * rayDirX;
    wallX -= std::floor(wallX);

    // x coordinate on the texture
    int texX = int(wallX * float(texWidth));
    if (side == 0 && rayDirX > 0)
      texX = texWidth - texX - 1;
    if (side == 1 && rayDirY < 0)
      texX = texWidth - texX - 1;

    // How much to increase the texture coordinate per screen pixel
    float step = 1.0f * texHeight / lineHeight;
    // Starting texture coordinate
    float texPos = (drawStart - screenHeight / 2 + lineHeight / 2) * step;

    // Get pixel array from surface
    Uint32 *pixels = nullptr;
    if (wallTexture) {
      pixels = (Uint32 *)wallTexture->pixels;
    }

    for (int y = drawStart; y < drawEnd; y++) {
      int texY = (int)texPos % texHeight;
      if (texY < 0) texY = 0;
      texPos += step;

      if (wallTexture && pixels) {
        Uint32 pixelColor = pixels[texHeight * texY + texX];

        // Shading
        if (side == 1) {
          // Simple bitwise shading for performance (ARGB)
          pixelColor = (pixelColor & 0xFF000000) | ((pixelColor & 0x00FEFEFE) >> 1);
        }
        
        // Ensure opaque for walls (some textures might have weird alpha)
        pixelColor |= 0xFF000000;

        buffer[y * screenWidth + x] = pixelColor;
      } else {
        buffer[y * screenWidth + x] = 0xFFFF0000;
      }
    }

    // No need to lock/unlock if using optimized surfaces

    // Save distance to Z-Buffer
    zBuffer[x] = perpWallDist;
  }

  // Sprite Rendering (Items + Enemy)
  struct Sprite {
    float x, y;
    int type; // 0 for enemy, 1 for item
    float distance;
  };

  std::vector<Sprite> sprites;

  // Add enemy
  sprites.push_back({enemy.x, enemy.y, 0,
                     ((player.x - enemy.x) * (player.x - enemy.x) +
                      (player.y - enemy.y) * (player.y - enemy.y))});

  // Add items
  for (const auto &item : items) {
    if (!item.collected) {
      sprites.push_back({item.x, item.y, 1,
                         ((player.x - item.x) * (player.x - item.x) +
                          (player.y - item.y) * (player.y - item.y))});
    }
  }

  // Sort sprites from furthest to closest
  for (size_t i = 0; i < sprites.size(); i++) {
    for (size_t j = 0; j < sprites.size() - 1; j++) {
      if (sprites[j].distance < sprites[j + 1].distance) {
        std::swap(sprites[j], sprites[j + 1]);
      }
    }
  }

  for (const auto &sprite : sprites) {
    float spriteX = sprite.x - player.x;
    float spriteY = sprite.y - player.y;

    float invDet =
        1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);

    float transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
    float transformY =
        invDet * (-player.planeY * spriteX + player.planeX * spriteY);

    // Skip if sprite is behind the player
    if (transformY <= 0)
      continue;

    int spriteScreenX = int((screenWidth / 2) * (1 + transformX / transformY));

    // Prevent transformY from being too small (avoids division by zero and
    // extreme scaling)
    if (transformY < 0.1f)
      transformY = 0.1f;

    int spriteHeight =
        std::abs(int(static_cast<float>(screenHeight) / transformY));

    // Items are smaller so we draw them lower and smaller based on type
    int spriteVisualHeight = spriteHeight;
    int vMoveScreen = 0;
    if (sprite.type == 1) {
      spriteVisualHeight = spriteHeight / 4; // Items are 1/4 size
      vMoveScreen = int(spriteHeight / 2);   // Move them down to the floor
    }

    int drawStartY = -spriteVisualHeight / 2 + screenHeight / 2 + vMoveScreen;
    if (drawStartY < 0)
      drawStartY = 0;
    int drawEndY = spriteVisualHeight / 2 + screenHeight / 2 + vMoveScreen;
    if (drawEndY >= screenHeight)
      drawEndY = screenHeight - 1;

    int spriteWidth =
        std::abs(int(static_cast<float>(screenHeight) / transformY));
    if (sprite.type == 1) {
      spriteWidth = spriteWidth / 4;
    }

    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    if (drawStartX < 0)
      drawStartX = 0;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawEndX >= screenWidth)
      drawEndX = screenWidth - 1;

    // Select proper texture based on entity type
    SDL_Surface *tex = (sprite.type == 0) ? enemyTexture : itemTexture;
    int texWidth = tex ? tex->w : 64;
    int texHeight = tex ? tex->h : 64;

    Uint32 *pixels = nullptr;
    if (tex) {
      pixels = (Uint32 *)tex->pixels;
    }

    for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
      int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) *
                     texWidth / spriteWidth) /
                 256;

      if (transformY > 0 && stripe > 0 && stripe < screenWidth &&
          transformY < zBuffer[stripe]) {
        for (int y = drawStartY; y < drawEndY; y++) {
          int d =
              (y - vMoveScreen) * 256 - screenHeight * 128 +
              spriteVisualHeight * 128;
          int texY = ((d * texHeight) / spriteVisualHeight) / 256;
          if (texY < 0) texY = 0;
          if (texY >= texHeight) texY = texHeight - 1;

          if (tex && pixels) {
            Uint32 pixelColor = pixels[texWidth * texY + texX];
            // ARGB format: check if alpha is not zero (handle semi-transparency)
            if ((pixelColor >> 24) > 10) { 
              buffer[y * screenWidth + stripe] = pixelColor;
            }
          } else {
            // Fallback solid color
            buffer[y * screenWidth + stripe] = (sprite.type == 0) ? 0xFFFF00FF : 0xFF00FFFF;
          }
        }
      }
    }

    // No locking needed
  }

  // Update backbuffer and copy to screen
  if (screenTexture) {
    SDL_UpdateTexture(screenTexture, NULL, buffer.data(),
                      screenWidth * sizeof(Uint32));
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
  }
}
