#include "SPI.h"
#include "EEPROM.h"

// Created by Ben McLean
// based on code from http://lodev.org/cgtutor/raycasting.html

#include "Arduboy.h"

#define global_seed 42
#define mapWidth 24
#define mapHeight 24
#define w 128
#define h 64

Arduboy arduboy;

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

unsigned char worldMap (int x, int y) {
  if (x == 0 || y == 0) return 1; // collision detection not working for negative numbers yet
  if (x % 2 == 1 && y % 2 == 1) return 0;
  if (x % 2 == 0 && y % 2 == 0) return 1;
  if (hash(x, y, global_seed) % 3 == 1)
    return 1;
  return 0;
}

double posX = 1001.5, posY = 1001.5; //x and y start position
double dirX = 1, dirY = 0; //initial direction vector
double planeX = 0, planeY = 0.66; //the 2d raycaster version of camera plane
double currentTime = 0; //time of current frame
double oldTime = 0; //time of previous frame

void setup() {
  arduboy.begin();
}

void loop() {
  arduboy.clear();

  for (int x = 0; x < w; x++)
  {
    //calculate ray position and direction
    double cameraX = 2 * x / double(w) - 1; //x-coordinate in camera space
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

    int hit = 0; //was there a wall hit?
    int side; //was a NS or a EW wall hit?
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
    while (hit == 0)
    {
      //jump to next map square, OR in x-direction, OR in y-direction
      if (sideDistX < sideDistY)
      {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = 0;
      }
      else
      {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = 1;
      }
      //Check if ray has hit a wall
      if (worldMap(mapX, mapY) > 0) hit = 1;
    }
    //Calculate distance projected on camera direction (oblique distance will give fisheye effect!)
    if (side == 0) perpWallDist = (mapX - rayPosX + (1 - stepX) / 2) / rayDirX;
    else           perpWallDist = (mapY - rayPosY + (1 - stepY) / 2) / rayDirY;

    //Calculate height of line to draw on screen
    int lineHeight = (int)(h / perpWallDist);

    //calculate lowest and highest pixel to fill in current stripe
    int drawStart = -lineHeight / 2 + h / 2;
    if (drawStart < 0)drawStart = 0;
    int drawEnd = lineHeight / 2 + h / 2;
    if (drawEnd >= h)drawEnd = h - 1;

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
    if (worldMap(int(posX + dirX * moveSpeed), int(posY)) == false) posX += dirX * moveSpeed;
    if (worldMap(int(posX), int(posY + dirY * moveSpeed)) == false) posY += dirY * moveSpeed;
  }
  //move backwards if no wall behind you
  if (arduboy.pressed(DOWN_BUTTON))
  {
    if (worldMap(int(posX - dirX * moveSpeed), int(posY)) == false) posX -= dirX * moveSpeed;
    if (worldMap(int(posX), int(posY - dirY * moveSpeed)) == false) posY -= dirY * moveSpeed;
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

  //arduboy.setCursor(0, 0);
  //arduboy.print(String(posX) + F(", ") + String(posY));

  arduboy.display();
}