
/**
 * Modified from Hunter-Adams-RP2040-Demos/VGA_Graphics/Animation_Demo (vha3@cornell.edu)
 */

// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "sprites.h"
#include "whoosh_sound.h"
#include "shield_sound.h"
#include "fight.h"
#include "hit.h"

#include <emscripten.h>

EM_JS(void, fill_rect, (int x, int y, int w, int h, int color), {
  Module.fillRect(x, y, w, h, color);
});

void fillRect(int x, int y, int w, int h, int color) {
  fill_rect(x, y, w, h, color);
}

EM_JS(void, draw_rect, (int x, int y, int w, int h, int color), {
  Module.drawRect(x, y, w, h, color);
});

void drawRect(int x, int y, int w, int h, int color) {
  draw_rect(x, y, w, h, color);
}

//=== ===
// Define constants
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_MIDLINE_X 320

short ui_state = 0;
short winner = -1; //-1: undefined.  0: player 1.  1: player 2.

#define GROUND_HEIGHT 60
#define GROUND_LEVEL 480 - GROUND_HEIGHT
#define GROUND_LEFT 148
#define GROUND_RIGHT 640 - 148

// Max amount of sounds we can store and play back
#define max_sounds 50

unsigned int Theme_freq[32] = {587, 587, 622, 622, 440, 440, 0, 0, 175, 587, 247, 622, 440, 0, 0, 932, 1109, 1109, 622, 622, 440, 440, 0, 0, 1109, 587, 622, 247, 220, 0, 0, 294};
short Theme_id = 0;

unsigned int button_freq[1] = {1175}; //D6
unsigned int placeholder_freq[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned int *effect_freq;
short effect_len = 1;
short effect_id = -1;
short count_1 = 0;

short hit_x=-1, hit_y=-1, hit_w=0, hit_h=0; //for drawing attack hitboxes

static void trigger_effect(unsigned int *freq, short len)
{
  effect_freq = freq;
  effect_len = len;
  count_1 = 0;
  effect_id = 0;
}

static uint32_t last_update_time = 0;
static uint32_t elapsed_time_sec = 0;

static short tracked_key;

char WHITE='w', BLACK='b', RED = 'r', YELLOW = 'y';
;

// Possible Player States: 0 = Idle, 1 = Attack, 2 = Hurt, 3 = Die

typedef struct
{
  short x;
  short y;

  bool flip;
  short state;
  short frame;
  short attack_hitbox;
  short hp;
  Anim *head_anim;
  Anim *body_anim;

  short body;
  short shield;
} player;

typedef struct
{
  short x_off;
  short y_off;
  short w;
  short h;
} hitbox;

// hitboxes:   0: stand body, 1: stand hit, 2: stand feet, 3: stand head, 4: drop hit, 5: crouch body, 6: crouch hit, 7: upward hit
const hitbox hitboxes[] = {{28, 160, 60, 160}, {-28, 128, 84, 56}, {28, 0, 60, 0}, {28, 160, 60, 0}, {28, 0, 60, 20}, {28, 124, 60, 124}, {-28, 28, 72, 28}, {8, 200, 32, 40}};
const short active_frames[] = {-1, 5, -1, -1}; // active frames for each attack
short clouds_x = 320;

#define NUM_PLAYERS 2
player players[2] = {{640 / 4, GROUND_LEVEL, false, 0, 0, 0, 200, E, C2, 0, 3}, {640 * 3 / 4, GROUND_LEVEL, true, 0, 0, 0, 200, A, C1, 0, 3}};

void resetGame()
{
  // winner=-1;
  players[0].x = 640 / 4;
  players[0].y = GROUND_LEVEL;
  players[0].flip = false;
  players[0].state = 0;
  players[0].frame = 0;
  players[0].attack_hitbox = 0;
  players[0].hp = 200;
  players[0].body = 0;
  players[0].shield = 3;

  players[1].x = 640 * 3 / 4;
  players[1].y = GROUND_LEVEL;
  players[1].flip = true;
  players[1].state = 0;
  players[1].frame = 0;
  players[1].attack_hitbox = 0;
  players[1].hp = 200;
  players[1].body = 0;
  players[1].shield = 3;
}



// Draw health bars for both players
void drawHealthBars(char color)
{
  if (players[0].hp<0)
    players[0].hp=0;
  if (players[1].hp<0)
    players[1].hp=0;
  fillRect(12, 20, 4, 16, color);                                  // left bar
  fillRect(224, 20, 4, 16, color);                                 // right bar
  fillRect(20 + players[0].hp, 28, 200 - players[0].hp, 4, color); // empty part
  fillRect(20, 20, players[0].hp, 16, color);                      // filled part

  fillRect(412, 20, 4, 16, color);                             // left bar
  fillRect(624, 20, 4, 16, color);                             // right bar
  fillRect(420, 28, 200 - players[1].hp, 4, color);            // empty part
  fillRect(620 - players[1].hp, 20, players[1].hp, 16, color); // filled part
}

void drawShields(char color)
{
  for (short i = 0, x = 12; i < 4; i++, x += 52)
  {
    fillRect(x, 40, 4, 8, color);
    fillRect(640 - x - 4, 40, 4, 8, color);
  }
  for (short i = 0, x = 20; i < players[0].shield; i++, x += 52)
  {
    fillRect(x, 40, 40, 8, color);
  }
  for (short i = 0, x = 580; i < players[1].shield; i++, x -= 52)
  {
    fillRect(x, 40, 40, 8, color);
  }
}

void eraseShields(bool p1)
{
  if (p1)
    for (short i = 0, x = 20; i < 3; i++, x += 52)
    {
      fillRect(x, 40, 40, 8, WHITE);
    }
  else
    for (short i = 0, x = 476; i < 3; i++, x += 52)
    {
      fillRect(x, 40, 40, 8, WHITE);
    }
}

void eraseHP(bool p1)
{
  if (p1)
  {
    fillRect(20, 20, 200, 16, WHITE);
    if (players[0].hp < 0)
      players[0].hp = 0;
  }
  else
  {
    fillRect(620 - 200, 20, 200, 16, WHITE);
    if (players[0].hp < 0)
      players[0].hp = 0;
  }
}

void drawLooped(const short arr[][2], short arr_len, short x, short y, char color)
{
  for (short i = 0; i < arr_len; i++)
  {
    if (x - arr[i][0] + 4 > 640)
      fillRect(x - arr[i][0] - 640, y - arr[i][1], 4, 4, color);
    else
      fillRect(x - arr[i][0], y - arr[i][1], 4, 4, color);
  }
}

void drawSprite(const short arr[][2], short arr_len, bool flip, short x, short y, char color)
{
  if (!flip)
    for (short i = 0; i < arr_len; i++)
      fillRect(x - arr[i][0], y - arr[i][1], 4, 4, color);
  else
    for (short i = 0; i < arr_len; i++)
      fillRect(x + arr[i][0], y - arr[i][1], 4, 4, color);
}

void drawFrame(player *p, char color)
{
  drawSprite(p->head_anim[p->state].f[p->frame].p, p->head_anim[p->state].f[p->frame].len, p->flip, p->x, p->y, color);
  drawSprite(p->body_anim[p->state].f[p->frame].p, p->body_anim[p->state].f[p->frame].len, p->flip, p->x, p->y, color);
}

void drawTitleScreen(bool ready)
{
  fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK); // set background to black
  short outline_off = 68;
  for(short i=0;i<2;i++)
  {
    if(players[i].head_anim==A)
    {
      if(winner<0)
        drawSprite(title_A_full, 2871, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT+8, WHITE);
      else if(winner!=i)
        drawSprite(title_A_lose, 1969, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT+8, WHITE);
      else
        drawSprite(title_A_win, 2807, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT+8, WHITE);
      drawSprite(title_A, 74, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
      drawSprite(A_Idle_0, 119, i==1,i==0?outline_off:SCREEN_WIDTH-outline_off, SCREEN_HEIGHT, BLACK);
    }
    else if (players[i].head_anim==E)
    {
      if(winner<0)
        drawSprite(title_E_full, 2070, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT+8, WHITE);
      else if(winner!=i)
        drawSprite(title_E_lose, 1597, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT+8, WHITE);
      else
        drawSprite(title_E_win, 3156, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT+8, WHITE);
      drawSprite(title_E, 70, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
      drawSprite(E_Idle_0, 72, i==1, i==0?outline_off:SCREEN_WIDTH-outline_off, SCREEN_HEIGHT, BLACK);
    }
    else
    {
      drawSprite(title_Z_full, 2439, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT+8, WHITE);
      drawSprite(title_Z, 91, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
      drawSprite(Z_Idle_0, 97, i==1, i==0?outline_off:SCREEN_WIDTH-outline_off, SCREEN_HEIGHT, BLACK);
    }
    if(players[i].body_anim==C1)
    {
      drawSprite(title_c1, 446, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
      drawSprite(C1_Idle_0, 141, i==1, i==0?outline_off:SCREEN_WIDTH-outline_off, SCREEN_HEIGHT, BLACK);
    }
    else if (players[i].body_anim==C2)
    {
      drawSprite(title_c2, 407, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
      drawSprite(C2_Idle_0, 147, i==1, i==0 ? outline_off:SCREEN_WIDTH-outline_off, SCREEN_HEIGHT, BLACK);
    }
    else
    {
      drawSprite(title_c3, 421, i==1, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
      drawSprite(C3_Idle_0, 134, i==1, i==0 ? outline_off:SCREEN_WIDTH-outline_off, SCREEN_HEIGHT, BLACK);
    }
      
  }
  if(ready)
  {
    drawSprite(title_ready, 1236, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
    drawSprite(title_ready_in, 428, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, BLACK);
    drawSprite(key_in,140, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, BLACK);
    drawSprite(key_in,140, true, SCREEN_MIDLINE_X-4, SCREEN_HEIGHT, BLACK);
  }
  else
  {
    drawSprite(title, 1235, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
    drawSprite(title_vs, 93, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
  }
}

void drawPauseScreen()
{
  // drawTitleScreen(true);
  // drawHealthBars(WHITE);
  // drawShields(WHITE);
  drawSprite(paused, 437, false,SCREEN_MIDLINE_X,SCREEN_HEIGHT,BLACK);
  drawSprite(key_out, 48, false,SCREEN_MIDLINE_X,SCREEN_HEIGHT,BLACK);
  drawSprite(key_out, 48, true,SCREEN_MIDLINE_X,SCREEN_HEIGHT,BLACK);
  drawSprite(key_in, 140, false,SCREEN_MIDLINE_X,SCREEN_HEIGHT,WHITE);
  drawSprite(key_in, 140, true,SCREEN_MIDLINE_X,SCREEN_HEIGHT,WHITE);
}

void drawBodyHitbox(short h, short p, char color)
{
  short h1xL = players[p].x - hitboxes[h].x_off;
  short h1xR = players[p].x + hitboxes[h].x_off - hitboxes[h].w;

  short h1y1 = players[p].y - hitboxes[h].y_off;
  short h1y2 = h1y1 + hitboxes[h].h;

  short h1x1;
  short h1x2;

  if (!players[p].flip)
  {
    h1x1 = h1xL;
    h1x2 = h1xL + hitboxes[h].w;
  }
  else
  {
    h1x1 = h1xR;
    h1x2 = h1xR + hitboxes[h].w;
  }

  drawRect(h1x1, h1y1, h1x2-h1x1, h1y2-h1y1, color);
  drawRect(h1x1+1, h1y1+1, h1x2-h1x1-2, h1y2-h1y1-2, color);
  drawRect(h1x1+2, h1y1+2, h1x2-h1x1-4, h1y2-h1y1-4, color);
}


bool isOverlapping(short h1, short h2, short attacker)
{
  if (h1 < 0 || h2 < 0)
    return false;
  // short h1x = players[attacker].x-hitboxes[h1].x_off;
  // short h1y = players[attacker].x-hitboxes[h1].y_off;

  short h1xL = players[attacker].x - hitboxes[h1].x_off;
  short h1xR = players[attacker].x + hitboxes[h1].x_off - hitboxes[h1].w;
  short h2xL = players[!attacker].x - hitboxes[h2].x_off;
  short h2xR = players[!attacker].x + hitboxes[h2].x_off - hitboxes[h2].w;

  short h1y1 = players[attacker].y - hitboxes[h1].y_off;
  short h1y2 = h1y1 + hitboxes[h1].h;
  short h2y1 = players[!attacker].y - hitboxes[h2].y_off;
  short h2y2 = h2y1 + hitboxes[h2].h;
  // short h1y = players[attacker].y;
  // short h2y = players[!attacker].y;

  short h1x1;
  short h1x2;
  short h2x1;
  short h2x2;

  if (!players[attacker].flip)
  {
    h1x1 = h1xL;
    h1x2 = h1xL + hitboxes[h1].w;
  }
  else
  {
    h1x1 = h1xR;
    h1x2 = h1xR + hitboxes[h1].w;
  }
  if (!players[!attacker].flip)
  {
    h2x1 = h2xL;
    h2x2 = h2xL + hitboxes[h2].w;
  }
  else
  {
    h2x1 = h2xR;
    h2x2 = h2xR + hitboxes[h2].w;
  }

  // return ((h1x>h2x && h1x<h2x+hitboxes[h2].w)||(h1x+hitboxes[h1].w>h2x && h1x<h2x+hitboxes[h2].w)) && ((h1y<h2y && h1y>h2y-hitboxes[h2].h)||(h1y-hitboxes[h1].h<h2y && h1y-hitboxes[h1].h>h2y-hitboxes[h2].h));
  bool x_overlap = (h1x1 >= h2x1 && h1x1 <= h2x2) || (h1x2 >= h2x1 && h1x2 <= h2x2) || (h2x1 >= h1x1 && h2x1 <= h1x2) || (h2x2 >= h1x1 && h2x2 <= h1x2);
  bool y_overlap = (h1y1 >= h2y1 && h1y1 <= h2y2) || (h1y2 >= h2y1 && h1y2 <= h2y2) || (h2y1 >= h1y1 && h2y1 <= h1y2) || (h2y2 >= h1y1 && h2y2 <= h1y2);
  bool r = x_overlap && y_overlap;

  if(r && h1!=0 && h1!=5)
  {
    hit_x=h1x1;
    hit_y=h1y1;
    hit_w=h1x2-h1x1;
    hit_h=h1y2-h1y1;
    // drawRect(h1x1,h1y1,h1x2-h1x1,h1y2-h1y1, YELLOW);
    // drawRect(h1x1+1,h1y1+1,h1x2-h1x1-2,h1y2-h1y1-2, YELLOW);
    // drawRect(h1x1+2,h1y1+2,h1x2-h1x1-4,h1y2-h1y1-4, YELLOW);
  }

  // if(x_overlap)
  //   printf("x_overlap\n");
  // if(y_overlap)
  //   printf("y_overlap:%d, %d, %d, %d\n", h1y1, h1y2, h2y1, h2y2);
  // else
  //   printf("missed y:%d, %d, %d, %d\n", h1y1, h1y2, h2y1, h2y2);
  // if(r)
  //   printf("overlap\n");
  return r;
}

short getKey(bool p1)
{
  return 0;
}

void handle_input_floating(short i)
{
  short tracked_key = getKey(i == 0);
  switch (tracked_key)
  {
  case 4: // left
  {
    bool overlap_before = isOverlapping(players[i].body, players[!i].body, i);
    players[i].x -= 10;
    if (players[i].x < GROUND_LEFT || (!overlap_before && isOverlapping(players[i].body, players[!i].body, i)))
      players[i].x += 10;
    break;
  }
  case 6: // right
  {
    bool overlap_before = isOverlapping(players[i].body, players[!i].body, i);
    players[i].x += 10;
    if (players[i].x > GROUND_RIGHT || (!overlap_before && isOverlapping(players[i].body, players[!i].body, i)))
      players[i].x -= 10;
    break;
  }
  case 7: // pause game
  {
    ui_state = 4; //go to pause state
    break;
  }
  default:
  {
    break;
  }
  }
}

void handle_input(short i)
{
  short tracked_key = getKey(i == 0);
  switch (tracked_key)
  {
  case 4: // left
  {
    bool overlap_before = isOverlapping(players[i].body, players[!i].body, i);
    players[i].x -= 10;
    if (players[i].x < GROUND_LEFT || (!overlap_before && isOverlapping(players[i].body, players[!i].body, i)))
      players[i].x += 10;
    players[i].state = players[i].flip ? 2 : 3;
    break;
  }
  case 6: // right
  {
    bool overlap_before = isOverlapping(players[i].body, players[!i].body, i);
    players[i].x += 10;
    if (players[i].x > GROUND_RIGHT || (!overlap_before && isOverlapping(players[i].body, players[!i].body, i)))
      players[i].x -= 10;
    players[i].state = players[i].flip ? 3 : 2;
    break;
  }
  case 5: // down
  {
    players[i].frame = 0;
    players[i].state = 8; // crouch state;
    break;
  }
  case 3: // upward punch
  {
    // trigger_effect(placeholder_freq,4);
    players[i].frame = 0;
    players[i].state = 10;
    break;
  }
  case 1: // attack
  {
    players[i].state = players[i].state == 8 ? 9 : 1; // crouch=>crouch attack, else stand attack
    players[i].frame = 0;
    break;
  }
  case 2: // jump
  {
    players[i].state = 5; // jump state
    players[i].frame = 0;
    break;
  }
  case 7: // pause game
  {
    ui_state = 4; //go to pause state
    break;
  }
  default:
  {
    players[i].state = 0; // idle state
    break;
  }
  }
}

void game_step()
{
  drawBodyHitbox(players[0].body,0,WHITE);//erase
  drawBodyHitbox(players[1].body,1,WHITE);//erase
  drawRect(hit_x, hit_y, hit_w, hit_h, WHITE); //current hit box
  drawRect(hit_x+1, hit_y+1, hit_w-2, hit_h-2, WHITE); //make it thicker
  drawRect(hit_x+2, hit_y+2, hit_w-4, hit_h-4, WHITE); 

  drawHealthBars(BLACK);
  drawShields(BLACK);
  // drawRect(80, 0 ,640-160, 480, BLACK);

  players[0].body = players[0].state == 11 ? -1 : players[0].state == 8 || players[0].state == 9 ? 5
                                                                                                 : 0; // none if dead, crouch body if crouch OR crouch attack, stand body otherwise
  players[1].body = players[1].state == 11 ? -1 : players[1].state == 8 || players[1].state == 9 ? 5
                                                                                                 : 0; // none if dead, crouch body if crouch OR crouch attack, stand body otherwise

  drawSprite(P1, 13, false, players[0].x, players[0].y, WHITE); // erase previous frame UI
  drawSprite(P2, 15, false, players[1].x, players[1].y, WHITE); // erase previous frame UI
  // drawLooped(clouds, 479, clouds_x, 480, WHITE); //erase previous clouds
  drawLooped(clouds3, 403, clouds_x, 480, WHITE); // erase previous clouds
  clouds_x++;
  if (clouds_x >= 960)
    clouds_x = 320;

  drawFrame(&players[0], WHITE); // player 0, erase previous frame
  drawFrame(&players[1], WHITE); // player 1, erase previous frame

  for (int i = 0; i < NUM_PLAYERS; i++)
  {
    switch (players[i].state)
    {
    case 12: // crouch guard
    {
      players[i].frame++;
      if (players[i].frame >= players[i].head_anim[players[i].state].len)
      {
        players[i].frame = 0;
        players[i].state = 8; // back to crouch state
      }
      break;
    }
    case 13: // stand guard
    {
      players[i].frame++;
      if (players[i].frame >= players[i].head_anim[players[i].state].len)
      {
        players[i].frame = 0;
        players[i].state = 0; // back to idle state
      }
      break;
    }

    case 8: // crouch
    case 0: // Idle
    case 2: // Forward
    case 3: // Backward
    {
      players[i].frame++;
      if (players[i].frame >= players[i].head_anim[players[i].state].len)
        players[i].frame = 0;

      // start falling if above ground && feet not on the other player
      if (players[i].y < GROUND_LEVEL && !isOverlapping(2, players[!i].body, i))
      {
        players[i].state = 6;
        players[i].frame = 0;
        break;
      }

      handle_input(i); // get keypresses
      break;
    }
    case 1: // stand attack
    {
      if(players[i].frame == 0) {
      }
      players[i].frame++;
      if (players[i].frame >= players[i].head_anim[1].len)
        players[i].frame = 0;

      // check if attack active
      if (players[i].frame == active_frames[players[i].state])
      {
        // trigger_effect(placeholder_freq,4);
        if (isOverlapping(1, players[!i].body, i))
        {
          // check back block
          if (players[!i].shield > 0 && players[!i].state == 3)
          {
            // dma_start_channel_mask(1u << shieldctrl_chan) ;
            players[!i].shield--;
            if (players[i].shield < 3)
              players[i].shield++;
            eraseShields((!i) == 0);
            players[!i].state = 13; // stand guard
            players[!i].frame = 0;
          }
          else
          {
            // dma_start_channel_mask(1u << hitctrl_chan) ;

            players[!i].frame = 0;
            players[!i].state = 4; // hurt state
            players[!i].hp -= 20;  // hurt state
            eraseHP((!i) == 0);
          }
        }
        else{
          // dma_start_channel_mask(1u << whooshctrl_chan) ;
        }
      }

      if (players[i].frame == 0)
        players[i].state = 0; // back to idle state
      break;
    }
    case 9: // crouch attack
    {

      players[i].frame++;
      if (players[i].frame >= players[i].head_anim[9].len)
      {
        players[i].frame = 0;
        players[i].state = 8; // back to crouch
      }
      // check if attack active
      if (players[i].frame == 2)
      {
        if (isOverlapping(6, players[!i].body, i))
        {
          // check crouch block
          if (players[!i].shield > 0 && players[!i].state == 8)
          {
            // dma_start_channel_mask(1u << shieldctrl_chan) ;
            players[!i].shield--;
            if (players[i].shield < 3)
              players[i].shield++;
            eraseShields((!i) == 0);
            players[!i].state = 12; // crouch guard
            players[!i].frame = 0;
          }
          else
          {
            // dma_start_channel_mask(1u << hitctrl_chan);

            players[!i].frame = 0;
            players[!i].state = 4; // hurt state
            players[!i].hp -= 10;  // hurt state
            eraseHP((!i) == 0);
          }
        }
        else{
          // dma_start_channel_mask(1u << whooshctrl_chan) ;
        }
      }
      break;
    }
    case 10: // upward attack
    {
      players[i].frame++;
      if (players[i].frame >= players[i].head_anim[10].len)
      {
        players[i].frame = 0;
        players[i].state = 0; // back to idle
      }
      // check if attack active
      if (players[i].frame == 3)
      {
        if (isOverlapping(7, players[!i].body, i))
        {
          // dma_start_channel_mask(1u << hitctrl_chan) ;
          players[!i].frame = 0;
          players[!i].state = 4; // hurt state
          players[!i].hp -= 20;  // hurt state
          eraseHP((!i) == 0);
        }
        else{
          // dma_start_channel_mask(1u << whooshctrl_chan) ;

        }
      }
      break;
    }
    case 4: // hurt state
    {
      if (players[i].hp <= 0)
      {
        players[i].frame = 0;
        players[i].state = 11; // dead state
        break;
      }


      players[i].frame++;
      if (players[i].frame >= players[i].head_anim[4].len)
      {
        players[i].frame = 0;
        players[i].state = 0; // back to idle state
      }
      break;
    }
    case 5: // jump state
    {
      if(players[i].frame == 0) {
        // dma_start_channel_mask(1u << whooshctrl_chan) ;
      }
      players[i].frame++;
      if (players[i].frame > 1)
      {
        bool overlap_before = isOverlapping(3, players[!i].body, i); // this.head vs. other.body
        players[i].y -= 40;
        bool overlap_after = isOverlapping(3, players[!i].body, i); // this.head vs. other.body

        if (!overlap_before && overlap_after)
          players[i].y = players[!i].y + hitboxes[0].h;
        handle_input_floating(i);
      }
      if (players[i].frame >= players[i].head_anim[5].len)
      {
        players[i].frame = 0;
        players[i].state = 6; // fall state
      }
      break;
    }
    case 6: // fall state
    {
      players[i].y += 40;
      if (isOverlapping(2, players[!i].body, i) && players[i].y < players[!i].y) // this.feet overlaps other.body && above other.feet
      {
        players[i].y = players[!i].y - hitboxes[players[!i].body].h;
        players[i].frame = 0;
        players[i].state = 7; // land state
        break;
      }
      else if (players[i].y >= GROUND_LEVEL) // this.feet vs. ground
      {
        players[i].y = GROUND_LEVEL;
        players[i].frame = 0;
        players[i].state = 7; // land state
        break;
      }
      handle_input_floating(i);
      break;
    }
    case 7: // land state
    {
      players[i].frame++;
      if (players[i].frame == 2)
      {
        if (isOverlapping(4, players[!i].body, i))
        {
          // check jump block
          if (players[!i].shield > 0 && players[!i].state == 5)
          {
            // dma_start_channel_mask(1u << shieldctrl_chan);

            players[!i].shield--;
            if (players[i].shield < 3)
              players[i].shield++;
            eraseShields((!i) == 0);
            players[!i].state = 6; // fall state
            players[!i].frame = 0; // fall state
            players[i].state = 5;  // jump state
            players[i].frame = 0;  // jump state
          }
          else
          {
            // dma_start_channel_mask(1u << hitctrl_chan);

            players[!i].frame = 0;
            players[!i].state = 4; // hurt state
            players[!i].hp -= 20;  // hurt state
            eraseHP((!i) == 0);
          }
        }
      }
      if (players[i].frame >= players[i].head_anim[7].len)
      {
        players[i].frame = 0;
        players[i].state = 0; // idle state
      }
      break;
    }
    case 11: // dead state
    {
      if (players[i].frame + 1 < players[i].head_anim[11].len)
        players[i].frame++;
      else
        ui_state = 3; // only transition to game over screen after playing dead animation
      // start falling if above ground && feet not on the other player
      if (players[i].y < GROUND_LEVEL && !isOverlapping(2, players[!i].body, i))
      {
        players[i].y += 40;
        if (isOverlapping(2, players[!i].body, i) && players[i].y < players[!i].y) // this.feet overlaps other.body && above other.feet
        {
          players[i].y = players[!i].y - hitboxes[players[!i].body].h;
        }
        else if (players[i].y >= GROUND_LEVEL) // this.feet vs. ground
        {
          players[i].y = GROUND_LEVEL;
        }
      }
      break;
    }
    default:
    {
      break;
    }
    }

    if (players[i].state != 11) // turn towards the other player if not dead
      players[i].flip = players[i].x > players[!i].x;
  }

  drawFrame(&players[0], BLACK); // player i, draw current frame
  drawFrame(&players[1], BLACK); // player i, draw current frame

  drawBodyHitbox(players[0].body,0,RED);//draw body hitbox
  drawBodyHitbox(players[1].body,1,RED);

  drawSprite(P1, 13, false, players[0].x, players[0].y, BLACK);
  drawSprite(P2, 15, false, players[1].x, players[1].y, BLACK);
  drawSprite(P1, 13, false, 264, 228, BLACK);
  drawSprite(P2, 15, false, 372, 228, BLACK);

  drawSprite(stars2, 15, false, 320, 480, BLACK);
  drawSprite(moon, 171, false, 320, 480, BLACK);
  drawLooped(clouds3, 403, clouds_x, 480, BLACK);
  drawLooped(clouds3_inside, 515, clouds_x, 480, WHITE);
  drawSprite(roof_decoration, 39, false, 324, 480, BLACK);
  drawSprite(roof_decoration, 39, true, 316, 480, BLACK);

  drawRect(hit_x, hit_y, hit_w, hit_h, YELLOW); //current hit box
  drawRect(hit_x+1, hit_y+1, hit_w-2, hit_h-2, YELLOW); //make it thicker
  drawRect(hit_x+2, hit_y+2, hit_w-4, hit_h-4, YELLOW); 
}
// core 1
void protothread_core1()
{
  // Variables for maintaining frame rate
  static int begin_time;
  static int spare_time;

  short p1_key_prev = -1;
  short p2_key_prev = -1;

  while (1)
  {
    // Measure time at start of thread

    // printf("%d\n",ui_state);

    switch (ui_state)
    {
    case 0://reset & draw title screen
    {
      winner=-1; //intentionally separated from resetGame
      resetGame();
      drawTitleScreen(false);
      ui_state = -1;
      break;
    }
    case -1: //wait to draw ready screen
    {
      if(getKey(true)>0 || getKey(false)>0) //press any key (except ESC) to enter ready screen
      {
        drawTitleScreen(true);
        ui_state = -2; 
      }
      break;
    }
    case -2: //in ready screen, display the keys being pressed
    {
      short p1_offset = 72;
      if(p1_key_prev>=0)
        drawSprite(key_sprites[p1_key_prev].p, key_sprites[p1_key_prev].len, false, SCREEN_MIDLINE_X-p1_offset, SCREEN_HEIGHT, BLACK);
      if(p2_key_prev>=0)
        drawSprite(key_sprites[p2_key_prev].p, key_sprites[p2_key_prev].len, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, BLACK);
        
      short p1_key = getKey(true);
      short p2_key = getKey(false);

      if(p1_key>=0)
      {
        drawSprite(key_sprites[p1_key].p, key_sprites[p1_key].len, false, SCREEN_MIDLINE_X-p1_offset, SCREEN_HEIGHT, WHITE);
        if (p1_key!=p1_key_prev)
        {
          trigger_effect(button_freq,1);
          switch(p1_key)
          {
            case 8:
            {
              winner=-1; // reset winner after switching character
              if(players[0].head_anim==E)
                players[0].head_anim=A;
              else if (players[0].head_anim==A)
                players[0].head_anim=Z;
              else
                players[0].head_anim=E;
              drawTitleScreen(true);
              break;
            }
            case 9:
            {
              if(players[0].body_anim==C1)
                players[0].body_anim=C2;
              else if (players[0].body_anim==C2)
                players[0].body_anim=C3;
              else
                players[0].body_anim=C1;
              drawTitleScreen(true);
              break;
            }
            default:
            {
              break;
            }
          }
        }
      }
        
      if(p2_key>=0)
      {
        drawSprite(key_sprites[p2_key].p, key_sprites[p2_key].len, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
        if(p2_key!=p2_key_prev)
        {
          trigger_effect(button_freq,1);
          switch(p2_key)
            {
              case 8:
              {
                winner=-1; // reset winner after switching character
                if(players[1].head_anim==E)
                  players[1].head_anim=A;
                else if (players[1].head_anim==A)
                  players[1].head_anim=Z;
                else
                  players[1].head_anim=E;
                drawTitleScreen(true);
                break;
              }
              case 9:
              {
                if(players[1].body_anim==C1)
                  players[1].body_anim=C2;
                else if (players[1].body_anim==C2)
                  players[1].body_anim=C3;
                else
                  players[1].body_anim=C1;
                drawTitleScreen(true);
                break;
              }
              default:
              {
                break;
              }
            }
          }
        }

      p1_key_prev = p1_key;
      p2_key_prev = p2_key;

      
      if(p1_key>0 && p1_key<7 && p1_key==p2_key) //start game when two players are pressing the same key in range [1,6]
      {
        ui_state = 2;
        fillRect(0, 0, 640, 480, WHITE);
        drawSprite(rooftop, 741, false, 322, 480, BLACK);
        drawSprite(rooftop, 741, true, 318, 480, BLACK);
        // dma_start_channel_mask(1u << fightctrl_chan) ;
      }
      else if(p1_key==0 && p2_key==0) // go back to title screen (and reset game) if both players are pressing ESC (key 0)
      {
        ui_state=0;
      }
      break;
    }
    case 2:
      game_step(); // game step
      break;

    case 3: //win state
      winner = players[0].hp<=0?1:0;
      drawTitleScreen(true);
      resetGame();
      ui_state = -2; //go back to ready screen (will show winner there)
      break;
    case 4:
    {
      drawPauseScreen();
      short p1_offset = 68;
      if(p1_key_prev>=0)
        drawSprite(key_sprites[p1_key_prev].p, key_sprites[p1_key_prev].len, false, SCREEN_MIDLINE_X-p1_offset, SCREEN_HEIGHT, WHITE);
      if(p2_key_prev>=0)
        drawSprite(key_sprites[p2_key_prev].p, key_sprites[p2_key_prev].len, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, WHITE);
        
      short p1_key = getKey(true);
      short p2_key = getKey(false);

      if(p1_key>=0)
        drawSprite(key_sprites[p1_key].p, key_sprites[p1_key].len, false, SCREEN_MIDLINE_X-p1_offset, SCREEN_HEIGHT, BLACK);
      if(p2_key>=0)
        drawSprite(key_sprites[p2_key].p, key_sprites[p2_key].len, false, SCREEN_MIDLINE_X, SCREEN_HEIGHT, BLACK);

      p1_key_prev = p1_key;
      p2_key_prev = p2_key;

      
      if(p1_key>0 && p1_key<7 && p1_key==p2_key) //start game when two players are pressing the same key in range [1,6]
      {
        ui_state = 2;
        fillRect(0, 0, 640, 480, WHITE);
        drawSprite(rooftop, 741, false, 322, 480, BLACK);
        drawSprite(rooftop, 741, true, 318, 480, BLACK);
        // dma_start_channel_mask(1u << fightctrl_chan) ;
      }
      else if(p1_key==0 && p2_key==0) // go back to title screen (and reset game) if both players are pressing ESC (key 0)
      {
        ui_state=0;
      }
      break;
    }
    default:
      break;
    }

    // yield for necessary amount of time
    
    // NEVER exit while
  } // END WHILE(1)
} // animation thread

// ========================================
// === main
// ========================================
// USE ONLY C-sdk library

int main() 
{
  resetGame();
  drawTitleScreen(false);
  return 0;
}