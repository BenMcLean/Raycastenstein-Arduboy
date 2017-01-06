// Created by Ben McLean
// based on code from http://lodev.org/cgtutor/raycasting.html

#include "Arduboy2.h"

#define global_seed 42
#define mapWidth 24
#define mapHeight 24
#define screenWidth 128
#define screenHeight 64
#define drawDistance 8
#define twiceDrawDistance 16 // set to drawDistance * 2

Arduboy2 arduboy;

double posX = 1001.5, posY = 1001.5; //x and y start position
double dirX = 1, dirY = 0; //initial direction vector
double planeX = 0, planeY = 0.66; //the 2d raycaster version of camera plane
double currentTime = 0; //time of current frame
double oldTime = 0; //time of previous frame
int playerX = 0, playerY = 0;
boolean visible[twiceDrawDistance][twiceDrawDistance];

void setup() {
  arduboy.begin();
  updateVisible(playerX, playerY);
}

void loop() {
  int newX = int(posX + 0.5);
  int newY = int(posY + 0.5);
  if (playerX != newX || playerY != newY) updateVisible(newX, newY);
  playerX = newX;
  playerY = newY;

  arduboy.clear();
  for (int x = 0; x < screenWidth; x++)
  {
    //calculate ray position and direction
    double cameraX = 2 * x / double(screenWidth) - 1; //x-coordinate in camera space
    double rayPosX = posX;
    double rayPosY = posY;
    double rayDirX = dirX + planeX * cameraX;
    double rayDirY = dirY + planeY * cameraX;
    //which box of the map we're in
    int mapX = int(rayPosX);
    int mapY = int(rayPosY);
    
    //length of ray from current position to next x or y-side
    double sideDistX;
    double sideDistY;

    //length of ray from one x or y-side to next x or y-side
    double deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX));
    double deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY));
    double perpWallDist;

    //what direction to step in x or y-direction (either +1 or -1)
    int stepX;
    int stepY;

    boolean hit = false; //was there a wall hit?
    boolean side = false; //was a NS or a EW wall hit?
    //calculate step and initial sideDist
    if (rayDirX < 0)
    {
      stepX = -1;
      sideDistX = (rayPosX - mapX) * deltaDistX;
    }
    else
    {
      stepX = 1;
      sideDistX = (mapX + 1.0 - rayPosX) * deltaDistX;
    }
    if (rayDirY < 0)
    {
      stepY = -1;
      sideDistY = (rayPosY - mapY) * deltaDistY;
    }
    else
    {
      stepY = 1;
      sideDistY = (mapY + 1.0 - rayPosY) * deltaDistY;
    }
    //perform DDA
    while (hit == false)
    {
      //jump to next map square, OR in x-direction, OR in y-direction
      if (sideDistX < sideDistY)
      {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = false;
      }
      else
      {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = true;
      }
      //Check if ray has hit a wall
      //if (worldMap(mapX, mapY) > 0) hit = true;
      //if (getVisible(mapX, mapY)) hit = true;
      hit = getVisible(mapX, mapY);
    }
    //Calculate distance projected on camera direction (oblique distance will give fisheye effect!)
    if (side == false) perpWallDist = (mapX - rayPosX + (1 - stepX) / 2) / rayDirX;
    else           perpWallDist = (mapY - rayPosY + (1 - stepY) / 2) / rayDirY;

    //Calculate height of line to draw on screen
    int lineHeight = (int)(screenHeight / perpWallDist);

    //calculate lowest and highest pixel to fill in current stripe
    int drawStart = -lineHeight / 2 + screenHeight / 2;
    if (drawStart < 0)drawStart = 0;
    int drawEnd = lineHeight / 2 + screenHeight / 2;
    if (drawEnd >= screenHeight)drawEnd = screenHeight - 1;

    //draw the pixels of the stripe as a vertical line
    arduboy.drawFastVLine(x, drawStart, drawEnd - drawStart, 1);
  }

  oldTime = currentTime;
  currentTime = millis();
  double frameTime = (currentTime - oldTime) / 1000.0; //frameTime is the time this frame has taken, in seconds

  //speed modifiers
  double moveSpeed = frameTime * 5.0; //the constant value is in squares/second
  double rotSpeed = frameTime * 3.0; //the constant value is in radians/second

  //move forward if no wall in front of you
  if (arduboy.pressed(UP_BUTTON))
  {
    if (!getVisible(int(posX + dirX * moveSpeed), int(posY))) posX += dirX * moveSpeed;
    if (!getVisible(int(posX), int(posY + dirY * moveSpeed))) posY += dirY * moveSpeed;
  }
  //move backwards if no wall behind you
  if (arduboy.pressed(DOWN_BUTTON))
  {
    if (!getVisible(int(posX - dirX * moveSpeed), int(posY))) posX -= dirX * moveSpeed;
    if (!getVisible(int(posX), int(posY - dirY * moveSpeed))) posY -= dirY * moveSpeed;
  }
  if (arduboy.pressed(LEFT_BUTTON))
  {
    //both camera direction and camera plane must be rotated
    double oldDirX = dirX;
    dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
    dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
    double oldPlaneX = planeX;
    planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
    planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
  }
  if (arduboy.pressed(RIGHT_BUTTON))
  {
    //both camera direction and camera plane must be rotated
    double oldDirX = dirX;
    dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
    dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
    double oldPlaneX = planeX;
    planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
    planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
  }

  arduboy.setCursor(0, 0);
  arduboy.print(String(posX) + F(", ") + String(posY));

  arduboy.display();
}

void updateVisible (int newX, int newY) {
  for (int x = 0; x < twiceDrawDistance; x++)
    for (int y = 0; y < twiceDrawDistance; y++)
      visible[x][y] = worldMap(
        newX - drawDistance + x,
        newY - drawDistance + y
        );
}

boolean getVisible (int x, int y) {
  return getVisible(x, y, playerX, playerY);
}

boolean getVisible(int x, int y, int mapX, int mapY) {
  return visible[mapX - x + drawDistance][mapY - y + drawDistance];
}

boolean worldMap (int x, int y) {
  if (x == 0 || y == 0) return true; // collision detection not working for negative numbers yet
  if (x % 2 == 1 && y % 2 == 1) return false;
  if (x % 2 == 0 && y % 2 == 0) return true;
  if (hash(x, y, global_seed) % 3 == 1)
    return true;
  return false;
}

unsigned long hash(int x, int y, int seed) {
  unsigned char c[10]; // 2 bytes for seed, 4 for x and y (16k seeds, map is 4.2b in each direction before repeats
  memcpy(c, &seed, 2);
  memcpy(c + 2, &x, 4);
  memcpy(c + 6, &y, 4);
  return APHash(c, 6);
}

unsigned int APHash(unsigned char* str, unsigned int len)
{
  unsigned int hash = 0xAAAAAAAA;
  unsigned int i    = 0;

  for (i = 0; i < len; str++, i++)
  {
    hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*str) * (hash >> 3)) :
            (~((hash << 11) + ((*str) ^ (hash >> 5))));
  }

  return hash;
}

