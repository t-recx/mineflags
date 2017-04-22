#include <stdio.h>
#include <string.h>
#include <allegro.h>
#include "agup-0.11/agup.h"
#include "agup-0.11/abeos.h"

#define MAX_FIELD_X 16
#define MAX_FIELD_Y 16

#define TILE_WATER 0
#define TILE_BLOCK 1
#define TILE_BOMB TILE_BLOCK+10

#define PLAYER_TYPE_HUMAN 0
#define PLAYER_TYPE_CPU 1

#define HUMAN 0
#define CPU 1

#define MSG_DCLOSE (MSG_USER+1) 

int take_screenshot ();
int d_mine_field_proc (int msg, DIALOG *d, int c);
int d_ptype_list_proc (int msg, DIALOG *d, int c);
int d_player_turns_proc (int msg, DIALOG *d, int c);
int d_decoration_proc (int msg, DIALOG *d, int c);
int d_image_press_button_proc (int msg, DIALOG *d, int c);
int d_windowed_button_proc (int msg, DIALOG *d, int c);
int d_sound_button_proc (int msg, DIALOG *d, int c);

int change_windowed (DIALOG *d);
int change_sound (DIALOG *d);

int get_out ();

int menu_screen ();

typedef struct
{
  int type;
  int points;
  char name[40];
} PLAYER;

PLAYER player[2]=
{
  { 0, 0, "Player1" },
  { 1, 0, "Player2" }
};

int turn= 0;

int play_again= 0;

int sound_status= 1;

volatile int cpu_time= 0;
int cpu_play_time= 0;

int mine_field[MAX_FIELD_Y][MAX_FIELD_X]=
{
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

int spoofed_mf[MAX_FIELD_Y][MAX_FIELD_X];

int windowed= 0;

char *ptype_list (int index, int *list_size);

FONT *big_font= NULL;
FONT *default_font= NULL;

SAMPLE *mine_sample= NULL;
SAMPLE *block_sample= NULL;

BITMAP *game_tiles[]=
{
  NULL,
  NULL,
  NULL
};

BITMAP *windowed_button[]=
{
  NULL,
  NULL
};

BITMAP *sound_button[]=
{
  NULL,
  NULL
};

BITMAP *flags_bmps[]=
{
  NULL,
  NULL
};

BITMAP *turnbox[]=
{
  NULL,
  NULL,
  NULL
};

DIALOG menu_dialog[]=
{
  /* dialog proc, x, y, w, h, fg, bg, key, flags, d1, d2, dp */
  { d_clear_proc, 0, 0, 320, 200, 0, 0, 0, 0, 0, 0, NULL },
  { d_ctext_proc, 160, 0, 0, 0, 16, -1, 0, 0, 0, 0, "MineFlags" },
  { d_agup_button_proc, 40, 80, 80, 20, 0, 0, 0, D_EXIT, 0, 0, "Start Game" },
  { d_agup_button_proc, 40, 80+30, 80, 20, 0, 0, 0, D_EXIT, 0, 0, "Instructions" },
  { d_agup_button_proc, 40, 80+30*2, 80, 20, 0, 0, 0, D_EXIT, 0, 0, "Quit Game" },
  { d_windowed_button_proc, 220, 80+30*2+20, 24, 24, 0, 0, 0, 0, 0, 0, "", NULL, change_windowed },
  { d_sound_button_proc, 220+40, 80+30*2+20, 24, 24, 0, 0, 0, 0, 0, 0, "", NULL, change_sound },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_F10, 0, take_screenshot },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_ESC, 0, get_out },
  { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL }
};

DIALOG login_dialog[]=
{
  /* dialog proc, x, y, w, h, fg, bg, key, flags, d1, d2, dp */
  { d_clear_proc, 0, 0, 320, 200, 0, 0, 0, 0, 0, 0, NULL },
  { d_ctext_proc, 160, 0, 0, 0, 16, -1, 0, 0, 0, 0, "Login" },
  { d_ctext_proc, 160-20-32, 75, 64, 12, 16, -1, 0, 0, 39, 0, "Player1" },
  { d_ctext_proc, 160+20+32, 75, 64, 12, 16, -1, 0, 0, 39, 0, "Player2" },
  { d_agup_edit_proc, 160-20-64, 90, 63, 16, 16, -1, 0, 0, 39, 0, player[0].name },
  { d_agup_edit_proc, 160+20, 90, 63, 16, 16, -1, 0, 0, 39, 0, player[1].name },
  { d_ptype_list_proc, 160-20-64, 90+20, 63, 32, 16, 0, 0, 0, 0, 0, ptype_list },
  { d_ptype_list_proc, 160+20, 90+20, 63, 32, 16, 1, 0, 0, 0, 0, ptype_list },
  { d_agup_button_proc, 160+20, 90+20+50, 64, 20, 16, 1, 0, D_EXIT, 0, 0, "Play >" },
  { d_agup_button_proc, 160-20-64, 90+20+50, 64, 20, 16, 1, 0, D_EXIT, 0, 0, "< Back" },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_F10, 0, take_screenshot },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_ESC, 0, get_out },
  { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL }
};

DIALOG instructions_dialog[]=
{
  /* dialog proc, x, y, w, h, fg, bg, key, flags, d1, d2, dp */
  { d_clear_proc, 0, 0, 320, 200, 0, 0, 0, 0, 0, 0, NULL },
  { d_bitmap_proc, 0, 0, 320, 200, 0, 0, 0, 0, 0, 0, NULL },
  { d_ctext_proc, 160, 0, 0, 0, 16, -1, 0, 0, 0, 0, "Instructions" },
  { d_text_proc, 20, 40, 0, 0, 16, -1, 0, 0, 0, 0, "Click a water tile to" },
  { d_text_proc, 22, 50, 0, 0, 16, -1, 0, 0, 0, 0, "unveil a block." },
  { d_text_proc, 110, 70+5, 0, 0, 16, -1, 0, 0, 0, 0, "Each number block indicates the" },
  { d_text_proc, 108, 80+5, 0, 0, 16, -1, 0, 0, 0, 0, "number of mines who surrounds it" },
  { d_text_proc, 108, 90+5, 0, 0, 16, -1, 0, 0, 0, 0, "diagonals count too!" },
  { d_text_proc, 20, 110+5*2, 0, 0, 16, -1, 0, 0, 0, 0, "The player who flags a mine" },
  { d_text_proc, 22, 120+5*2, 0, 0, 16, -1, 0, 0, 0, 0, "gets to play another time." },
  { d_text_proc, 110, 140+5*3, 0, 0, 16, -1, 0, 0, 0, 0, "A player wins when" },
  { d_text_proc, 108, 150+5*3, 0, 0, 16, -1, 0, 0, 0, 0, "he flags 26 mines." },
  { d_text_proc, 108-8*10, 145+5*3, 6, 8*9, 255, 55, 0, 0, 0, 0, "Mines: 26" },
  { d_agup_button_proc, 160+20+64, 90+20+50, 64, 20, 16, 1, 0, D_EXIT, 0, 0, "< Back" },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_F10, 0, take_screenshot },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_ESC, 0, get_out },
  { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL }
};

DIALOG game_dialog[]=
{
  /* dialog proc, x, y, w, h, fg, bg, key, flags, d1, d2, dp */
  { d_clear_proc, 0, 0, 320, 200, 0, 0, 0, 0, 0, 0, NULL },
  { d_decoration_proc, 20, 4, 60, 120, 0, 0, 0, 0, 0, 0, NULL },
  { d_mine_field_proc, 110, 4, 12*16, 12*16, 0, 0, 0, 0, 0, 0, NULL },
  { d_player_turns_proc, 20, 4, 60, 120, 0, 0, 0, 0, 0, 0, NULL },
  { d_agup_button_proc, 20+5, 110+30*2, 80, 20, 0, 0, 0, D_EXIT, 0, 0, "Quit" },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_F10, 0, take_screenshot },
  { d_keyboard_proc, 0, 0, 0, 0, 0, 0, 0, 0, KEY_ESC, 0, get_out },
  { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL }
};

void increment_cpu_time ()
{
  cpu_time++;
}
END_OF_FUNCTION (increment_cpu_time);

int get_out ()
{
  return (D_CLOSE);
}

int change_windowed (DIALOG *d)
{
  return (D_CLOSE);
}

int change_sound (DIALOG *d)
{
  if (sound_status==0) { sound_status= 1; } else { sound_status= 0; }

  return (D_O_K);
}

int take_screenshot ()
{
  FILE *fd= NULL;
  BITMAP *save= NULL;
  char filename[200];
  int x=0;
  char a[210];

  save= create_sub_bitmap (screen, 0, 0, SCREEN_W, SCREEN_H);

  for (x=0;;x++)
  {
    sprintf (filename, "screenshot%d.bmp", x);
    fd= fopen (filename, "r");
    if (fd==NULL)
    {
      goto out;
    }
  }
out:
  sprintf (a, "as %s", filename);
  save_bitmap (filename, save, default_palette);
  alert ("Picture saved", a, "", "&Ok", NULL, 'o', 0);
  destroy_bitmap (save);
  if (fd!=NULL)
  {
    fclose (fd);
  }

  return (D_O_K);
}

int rand_ex (int min, int max)
{
  if (max<min)
  {
    return (0);
  }
  return (rand()% (max+1 - min)) + min;
}

char *ptype_list (int index, int *list_size)
{
  char *strings[]=
  {
    "Human",
    "Cpu"
  };
  if (index < 0)
  {
    *list_size= 2;
    return (NULL);
  }
  else
  {
    return (strings[index]);
  } 
}

void set_rand_mine_field ()
{
  int fx, fy;
  int nbombs= 0;

  srand (time (NULL));

  for (fy=0;fy<MAX_FIELD_Y;fy++)
  {
    for (fx=0;fx<MAX_FIELD_X;fx++)
    {
      mine_field[fy][fx]= 0;
    }
  }  

restart:
  for (fy=0;fy<MAX_FIELD_Y;fy++)
  {
    for (fx=0;fx<MAX_FIELD_X;fx++)
    {
      if (rand_ex (0+fx*fy, 5+fx*fy)-fx*fy==0 && mine_field[fy][fx]!=TILE_BOMB)
      {
        if (nbombs!=51)
        {
          mine_field[fy][fx]= TILE_BOMB;
          nbombs++;
        }
      }
    }
  }  

  if (nbombs!=51)
  {
    goto restart;
  }
}

void organize_mine_field ()
{
  int field_x, field_y;

  for (field_y=0;field_y<MAX_FIELD_Y;field_y++)
  {
    for (field_x=0;field_x<MAX_FIELD_X;field_x++)
    {
      if (mine_field[field_y][field_x]!=TILE_BOMB)
      {
        int number= 0;

        if (field_y>0)
        {
          if (mine_field[field_y-1][field_x]==TILE_BOMB)
          {
            number++;
          }
          if (field_x>0)
          {
            if (mine_field[field_y-1][field_x-1]==TILE_BOMB)
            {
              number++;
            }
          }
          if (field_x<MAX_FIELD_X-1)
          {
            if (mine_field[field_y-1][field_x+1]==TILE_BOMB)
            {
              number++;
            }
          }
        }
        if (field_y<MAX_FIELD_Y-1)
        {
          if (mine_field[field_y+1][field_x]==TILE_BOMB)
          {
            number++;
          }
          if (field_x>0)
          {
            if (mine_field[field_y+1][field_x-1]==TILE_BOMB)
            {
              number++;
            }
          }
          if (field_x<MAX_FIELD_X-1)
          {
            if (mine_field[field_y+1][field_x+1]==TILE_BOMB)
            {
              number++;
            }
          }
        }
        if (field_x>0)
        {
          if (mine_field[field_y][field_x-1]==TILE_BOMB)
          {
            number++;
          }
        }
        if (field_x<MAX_FIELD_X-1)
        {
          if (mine_field[field_y][field_x+1]==TILE_BOMB)
          {
            number++;
          }
        }

        mine_field[field_y][field_x]= TILE_BLOCK+number;
      }
    }
  }
}

int unspoof_block (int field_y, int field_x)
{
  if (mine_field[field_y][field_x]!=TILE_BOMB && 
      spoofed_mf[field_y][field_x]!=1)
  {
    if (mine_field[field_y][field_x]!=TILE_BLOCK)
    {
      spoofed_mf[field_y][field_x]= 1;
      return (0);
    }

    spoofed_mf[field_y][field_x]= 1;

    if (field_y>0)
    {
      unspoof_block (field_y-1, field_x);
      if (field_x>0)
      {
        unspoof_block (field_y-1, field_x-1);
      }
      if (field_x<MAX_FIELD_X-1)
      {
        unspoof_block (field_y-1, field_x+1);
      }
    }
    if (field_y<MAX_FIELD_Y-1)
    {
      unspoof_block (field_y+1, field_x);
      if (field_x>0)
      {
        unspoof_block (field_y+1, field_x-1);
      }
      if (field_x<MAX_FIELD_X-1)
      {
        unspoof_block (field_y+1, field_x+1);
      }
    }
    if (field_x>0)
    {
      unspoof_block (field_y, field_x-1);
    }
    if (field_x<MAX_FIELD_X-1)
    {
      unspoof_block (field_y, field_x+1);
    }
  }

  return (0);
}

int unspoof (int field_y, int field_x)
{
  if (mine_field[field_y][field_x]!=TILE_BOMB && spoofed_mf[field_y][field_x]==0)
  {
    if (turn==0) { turn= 1; } else { turn= 0; }
  }

  switch (mine_field[field_y][field_x])
  {
    case TILE_BOMB:
      if (spoofed_mf[field_y][field_x]==0)
      {
        spoofed_mf[field_y][field_x]= 2+turn;
        player[turn].points++;
        if (sound_status)
        {
          play_sample (mine_sample, 128, 128, 1000, 0);
        }
        return (1);
      }
      break;

    case TILE_BLOCK:   
      if (spoofed_mf[field_y][field_x]==0)
      {
        unspoof_block (field_y, field_x);
        if (sound_status)
        {
          play_sample (block_sample, 128, 128, 1000, 0);
        }
      }
      break;

    default:
      if (spoofed_mf[field_y][field_x]==0)
      {
        if (sound_status)
        {
          play_sample (block_sample, 128, 128, 1000, 0);
        }
      }
      spoofed_mf[field_y][field_x]= 1;
  }
  return (0);
}

int get_random_mf_cpu_play (int x, int y, int *fx, int *fy)
{
  typedef struct
  {
    int x, y;
  } _CHECK_BLOCK;
  int block_number= mine_field[y][x]-1, pblock= 0, c= 0;
  _CHECK_BLOCK possible_block[8];
  _CHECK_BLOCK check_block[8]= 
  {
    { -1, -1 },
    { -1,  0 },
    {  0, -1 },
    {  1,  1 },
    {  1,  0 },
    {  0,  1 },
    { -1,  1 },
    {  1, -1 }
  };

  /* check surroundings */
  for (c=0;c<8;c++)
  {
    if (spoofed_mf[y+check_block[c].y][x+check_block[c].x])
    {
      if (x+check_block[c].x>=0 && x+check_block[c].x<MAX_FIELD_X)
      {
        if (y+check_block[c].y>=0 && y+check_block[c].y<MAX_FIELD_Y)
        {
          if (mine_field[y+check_block[c].y][x+check_block[c].x]==TILE_BOMB)
          {
            block_number--;
          }
        }
      }
    }
    else
    {
      if (x+check_block[c].x>=0 && x+check_block[c].x<MAX_FIELD_X)
      {
        if (y+check_block[c].y>=0 && y+check_block[c].y<MAX_FIELD_Y)
        {
          possible_block[pblock].x= x+check_block[c].x;
          possible_block[pblock].y= y+check_block[c].y;
          pblock++;
        }
      }
    }
  }

  if (block_number>0 && pblock>0)
  {
    int rb;

    rb= rand_ex (0, pblock-1);
    *fx= possible_block[rb].x;
    *fy= possible_block[rb].y;

    return (1);
  }

  return (0);
}

int get_cpu_play (int *fx, int *fy)
{
  typedef struct
  {
    int fx, fy;
  } _FREE_BLOCK;
  _FREE_BLOCK free_block[MAX_FIELD_X*MAX_FIELD_Y];
  int x, y, block= 0, random_block;

  for (y=0;y<MAX_FIELD_Y;y++)
  {
    for (x=0;x<MAX_FIELD_X;x++)
    {
      if (spoofed_mf[y][x]==0)
      {
        free_block[block].fx= x;
        free_block[block].fy= y;
        block++;
      }
    }
  }

  for (y=0;y<MAX_FIELD_Y;y++)
  {
    for (x=0;x<MAX_FIELD_X;x++)
    {
      if (spoofed_mf[y][x]==1)
      {
        if (mine_field[y][x]>=TILE_BLOCK && mine_field[y][x]<=TILE_BLOCK+9)
        {
          if (get_random_mf_cpu_play (x, y, fx, fy))
          {
            return (0);
          }
        }
      }
    }
  }

  random_block= rand_ex (0, block-1);
  *fx= free_block[random_block].fx; *fy= free_block[random_block].fy;

  return (0);
}

int d_ptype_list_proc (int msg, DIALOG *d, int c)
{
  int ret;

  switch (msg)
  {
    case MSG_CLICK:
      ret= d_agup_list_proc (msg, d, c);
      if (d->d1==1)
      {
        login_dialog[4+d->bg].flags|= D_DISABLED;
        login_dialog[4+d->bg].flags|= D_DIRTY;
        login_dialog[4+d->bg].d2= 3;
        player[d->bg].type= 1;
        strcpy (login_dialog[4+d->bg].dp, "CPU");
      }
      else
      {
        login_dialog[4+d->bg].flags&= ~D_DISABLED;
        login_dialog[4+d->bg].flags|= D_DIRTY;
        login_dialog[4+d->bg].d2= 3;
        player[d->bg].type= 0;
      }
      return (ret);
      break;
  }

  return (d_agup_list_proc (msg, d, c));
}

int d_image_press_button_proc (int msg, DIALOG *d, int c)
{
  BITMAP *blah= d->dp2;
  int x, y;

  d->dp= "";

  switch (msg)
  {
    case MSG_DRAW:
      d_agup_button_proc (MSG_DRAW, d, 0);
      x= d->x+d->w/2-blah->w/2;
      y= d->y+d->h/2-blah->h/2;
      if (d->flags & D_SELECTED) { x++; y++; }
      draw_sprite (screen, d->dp2, x, y);
      return (D_O_K);
      break;
  }

  return (d_agup_push_proc (msg, d, c));
}

int d_mine_field_proc (int msg, DIALOG *d, int c)
{
  int rtm;
  int field_x, field_y;
  int field_w= d->w/16;
  int field_h= d->h/16;

  switch (msg)
  {
    case MSG_DRAW:
      for (field_y=0;field_y<16;field_y++)
      {
        for (field_x=0;field_x<16;field_x++)
        {
          if (spoofed_mf[field_y][field_x])
          {
            switch (mine_field[field_y][field_x])
            {
              case TILE_BLOCK ... TILE_BLOCK+9:
                draw_sprite (screen, game_tiles[TILE_BLOCK], d->x+field_x*field_w, d->y+field_y*field_h);
                break;
              case TILE_BOMB:
                draw_sprite (screen, flags_bmps[spoofed_mf[field_y][field_x]-2], d->x+field_x*field_w, d->y+field_y*field_h);
                draw_sprite (screen, flags_bmps[spoofed_mf[field_y][field_x]-2], d->x+field_x*field_w, d->y+field_y*field_h);
                break;
            }
            rtm= text_mode (-1);
            if (mine_field[field_y][field_x]>1 && mine_field[field_y][field_x]<11)
            {
              textprintf (screen, default_font, 
                  d->x+field_x*field_w+game_tiles[0]->w/2-text_length (default_font, " ")/2,
                  d->y+field_y*field_h+game_tiles[0]->h/2-text_height (default_font)/2,
                  16, "%d", mine_field[field_y][field_x]-1);
            }
            text_mode (rtm);
          }
          else
          {
            draw_sprite (screen, game_tiles[TILE_WATER], d->x+field_x*field_w, d->y+field_y*field_h);
          }
        }
      }
      break;

    case MSG_LPRESS:
      if (player[turn].type!=HUMAN)
      {
        return (D_O_K);
      }

      for (field_y=0;field_y<16;field_y++)
      {
        for (field_x=0;field_x<16;field_x++)
        {
          if (mouse_x > d->x+field_x*field_w && mouse_y > d->y+field_y*field_h &&
              mouse_x <= d->x+field_x*field_w+game_tiles[0]->w && mouse_y <= d->y+field_y*field_h+game_tiles[0]->h)
          {
            if (unspoof (field_y, field_x))
            {
              cpu_time= 0;
              cpu_play_time= rand_ex (500+field_x, 2000+field_x)-field_x;
            }
            d->flags|= D_DIRTY;
          }
        }
      }
      break;

    case MSG_IDLE:
      if (player[turn].type==CPU && player[turn].points<26 && cpu_time>=cpu_play_time)
      {
        int fx= 0, fy= 0;
        get_cpu_play (&fx, &fy);
        unspoof (fy, fx);
        d->flags|= D_DIRTY;
        cpu_time= 0;
        cpu_play_time= rand_ex (500+fx, 1500+fx)-fx;
      }
      break;

    case MSG_DCLOSE:
      return (D_CLOSE);
      break;
  }

  return (D_O_K);
}

int d_windowed_button_proc (int msg, DIALOG *d, int c)
{
  switch (msg)
  {
    case MSG_START:
    case MSG_IDLE:
      if (d->dp2!=windowed_button[windowed])
      {
        d->dp2= windowed_button[windowed];
        return (D_REDRAWME);
      }
      break;
  }

  return (d_image_press_button_proc (msg, d, c));
}

int d_sound_button_proc (int msg, DIALOG *d, int c)
{
  switch (msg)
  {
    case MSG_START:
    case MSG_IDLE:
      if (d->dp2!=sound_button[sound_status])
      {
        d->dp2= sound_button[sound_status];
        return (D_REDRAWME);
      }
      break;
  }

  return (d_image_press_button_proc (msg, d, c));
}

int get_unspoofed_bombs ()
{
  int fx, fy, ubombs= 0;

  for (fy=0;fy<MAX_FIELD_Y;fy++)
  {
    for (fx=0;fx<MAX_FIELD_X;fx++)
    {
      if (spoofed_mf[fy][fx]>1)
      {
        ubombs++;
      }
    }
  }
  return (ubombs);
}

int d_decoration_proc (int msg, DIALOG *d, int c)
{
  if (msg==MSG_DRAW)
  {
    draw_sprite (screen, turnbox[2], d->x, d->y);
  }
  else if (msg==MSG_DCLOSE)
  {
    return (D_CLOSE);
  }

  return (D_O_K);
}

char *get_underlines (int n)
{
  int c;
  char string[n+1];

  strcpy (string, "");

  for (c=0;c<n;c++)
  {
    char blah[2];
    strcpy (blah, "_");
    strcat (string, blah);
  }

  return (strdup (string));
}

int d_player_turns_proc (int msg, DIALOG *d, int c)
{
  int rtm;
  int t;
  char player1vic[60], player2vic[60];
  char player1los[60], player2los[60];

  switch (msg)
  {
    case MSG_DRAW:
      draw_sprite (screen, turnbox[0], d->x+5, d->y+2);
      draw_sprite (screen, turnbox[1], d->x+5, d->y+2+10+turnbox[0]->h);

      rtm= text_mode (-1);
      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+5, 255, "%s", player[0].name);
      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+10+5+turnbox[0]->h+1, 255, "%s", player[1].name);

      sprintf (player1vic, "%spoints", player[0].name);
      sprintf (player2vic, "%spoints", player[1].name);
      sprintf (player1los, "%sloses", player[0].name);
      sprintf (player2los, "%sloses", player[1].name);

      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+5+20+1, 255, "Wins: %d", 
          get_config_int ("scores", player1vic, 0));
      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+1+20+5+10+turnbox[0]->h+1, 255, "Wins: %d",
          get_config_int ("scores", player2vic, 0));
      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+5-1+30, 255, "Loses: %d", 
          get_config_int ("scores", player1los, 0));
      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+30-1+5+10+turnbox[0]->h+1, 255, "Loses: %d",
          get_config_int ("scores", player2los, 0));

      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+5+40, 255, "Mines: %d", player[0].points);
      textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+40+5+10+turnbox[0]->h+1, 255, "Mines: %d", player[1].points);
      if (turn==0)
      {
        textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
            d->y+2+5+10, 255, "is playing");
      }
      else
      {
        textprintf_centre (screen, font, d->x+5+turnbox[0]->w/2+1, 
            d->y+2+10+10+5+turnbox[0]->h+1, 255, "is playing");
      }
      text_mode (rtm);

      textprintf_centre (screen, default_font, d->x+5+turnbox[0]->w/2+1, 
          d->y+2+turnbox[0]->h+1, 255, " %d ", 51-get_unspoofed_bombs ());
      break;

    case MSG_IDLE:
      if (d->d1!=get_unspoofed_bombs ())
      {
        d->d1= get_unspoofed_bombs ();
        d->flags|= D_DIRTY;
      }
      if (d->d2!=turn)
      {
        d->d2= turn;
        d->flags|= D_DIRTY;
      }
      for (t=0;t<2;t++)
      {
        if (player[t].points==26)
        {
          char blah[200];
          scare_mouse ();
          object_message (d, MSG_DRAW, 0);
          unscare_mouse ();
          sprintf (blah, "%s wins!", player[t].name);
          play_again= 0;
          if (alert ("", blah, "", "&Ok", "&Play Again", 'o', 'p')==2)
          {
            play_again= 1;
          }
          broadcast_dialog_message (MSG_DCLOSE, 0);
          return (D_CLOSE);
        }
      }
      break;
  }

  return (D_O_K);
}

int init_counters ()
{
  LOCK_VARIABLE (cpu_time);
  LOCK_FUNCTION (increment_cpu_time);

  install_int_ex (increment_cpu_time, MSEC_TO_TIMER (1));

  return (0);
}

int init_theme ()
{
  agup_init (abeos_theme);

  gui_fg_color = agup_fg_color;
  gui_bg_color = agup_bg_color;

  gui_shadow_box_proc = d_agup_shadow_box_proc;
  gui_button_proc = d_agup_button_proc;
  gui_edit_proc = d_agup_edit_proc;
  gui_list_proc = d_agup_list_proc;
  gui_text_list_proc = d_agup_text_list_proc;

  return (0);
}

int init_fonts ()
{
  DATAFILE *font_data= NULL;

  font_data= load_datafile ("fonts.dat");
  big_font= font_data[1].dat;
  default_font= font;
  font= font_data[0].dat;
  return (0);
}

int init_samples ()
{
  mine_sample= load_sample ("wav/mine.wav");
  block_sample= load_sample ("wav/block.wav");
  return (0);
}

int init_bitmaps ()
{
  game_tiles[TILE_WATER]= load_bitmap ("bmps/water.bmp", default_palette);
  game_tiles[TILE_BLOCK]= load_bitmap ("bmps/block.bmp", default_palette);
  game_tiles[TILE_BLOCK+1]= load_bitmap ("bmps/bomb.bmp", default_palette);
  flags_bmps[0]= load_bitmap ("bmps/blue_flag.bmp", default_palette);
  flags_bmps[1]= load_bitmap ("bmps/red_flag.bmp", default_palette);
  turnbox[0]= load_bitmap ("bmps/bluebox.bmp", default_palette);
  turnbox[1]= load_bitmap ("bmps/redbox.bmp", default_palette);
  turnbox[2]= load_bitmap ("bmps/mainbox.bmp", default_palette);
  windowed_button[0]= load_bitmap ("bmps/window.bmp", default_palette);
  windowed_button[1]= load_bitmap ("bmps/fullscreen.bmp", default_palette);
  sound_button[0]= load_bitmap ("bmps/soundsoff.bmp", default_palette);
  sound_button[1]= load_bitmap ("bmps/soundson.bmp", default_palette);
  instructions_dialog[1].dp= load_bitmap ("bmps/instructions.bmp", default_palette);

  return (0);
}

int init ()
{
  if (allegro_init ()!=0)
  {
    allegro_message ("Error initializing allegro: %s\n", allegro_error);
    allegro_exit ();
    return (-1);
  }

  if (install_sound (DIGI_AUTODETECT, MIDI_AUTODETECT, NULL)!=0)
  {
    allegro_message ("Error initializing sound: %s\n", allegro_error);
    allegro_exit ();
    return (-1);
  }

  if (install_keyboard ()!=0)
  {
    allegro_message ("Error initializing keyboard: %s\n", allegro_error);
    allegro_exit ();
    return (-1);
  }

  if (install_mouse ()==-1)
  {
    allegro_message ("Error initializing mouse: %s\n", allegro_error);
    allegro_exit ();
    return (-1);
  }

  if (install_timer()!=0)
  {
    allegro_message ("Error initializing timer: %s\n", allegro_error);
    allegro_exit ();
    return (-1);
  }

  strcpy (player[0].name, get_config_string ("startup", "p1name", "Player1"));
  strcpy (player[1].name, get_config_string ("startup", "p2name", "CPU"));
  player[0].type= get_config_int ("startup", "p1type", 0);
  player[1].type= get_config_int ("startup", "p2type", 1);
  sound_status= get_config_int ("startup", "sound", 1);
  windowed= get_config_int ("startup", "windowed", -1);

  if (windowed==-1)
  {
    windowed= 1;
    if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0)!=0)
    {
      windowed= 0;
      if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0)!=0)
      {
        allegro_message ("Error starting up graphics: %s\n", allegro_error);
        allegro_exit ();
        return (-1);
      }
    }
  }
  else
  {
    if (windowed)
    {
      windowed= 1;
      if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0)!=0)
      {
        windowed= 0;
        if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0)!=0)
        {
          allegro_message ("Error starting up graphics: %s\n", allegro_error);
          allegro_exit ();
          return (-1);
        }
      }
    }
    else
    {
      windowed= 0;
      if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0)!=0)
      {
        windowed= 1;
        if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0)!=0)
        {
          allegro_message ("Error starting up graphics: %s\n", allegro_error);
          allegro_exit ();
          return (-1);
        }
      }
    }
  }

  init_counters ();
  init_theme ();
  init_fonts ();
  init_samples ();
  init_bitmaps ();

  return (0);
}

void obfuscate_mine_field ()
{
  int fx, fy;

  for (fy=0;fy<MAX_FIELD_Y;fy++)
  {
    for (fx=0;fx<MAX_FIELD_X;fx++)
    {
      spoofed_mf[fy][fx]= 0;
    }
  }
}

void clear_player_scores ()
{
  player[0].points= 0;
  player[1].points= 0;
}

int game_screen ()
{
  int p;

  game_dialog[0].fg= gui_fg_color;
  game_dialog[0].bg= gui_bg_color;

  switch (do_dialog (game_dialog, -1))
  {
    case 3:
      for (p=0;p<2;p++)
      {
        if (player[p].points>=26)
        {
          char blah[200];

          sprintf (blah, "%spoints", player[p].name);
          set_config_int ("scores", blah, get_config_int ("scores", blah, 0)+1);
        }
        else
        {
          char blah[200];

          sprintf (blah, "%sloses", player[p].name);
          set_config_int ("scores", blah, get_config_int ("scores", blah, 0)+1);
        }
      }
      if (play_again)
      {
        set_rand_mine_field ();
        organize_mine_field ();
        obfuscate_mine_field ();
        clear_player_scores ();

        return (game_screen ());
      }
      return (menu_screen ());
      break;

    case 4:
      return (menu_screen ());
      break;

    case 6:
      return (menu_screen ());
      break;
  }

  return (game_screen ());
}

int instructions_screen ()
{
  instructions_dialog[0].fg= gui_fg_color;
  instructions_dialog[0].bg= gui_bg_color;
  instructions_dialog[1].fg= gui_fg_color;
  instructions_dialog[1].bg= gui_bg_color;
  instructions_dialog[2].dp2= big_font;

  switch (do_dialog (instructions_dialog, -1))
  {
    case 13:
    case 15:
      return (menu_screen ());
      break;
  }

  return (instructions_screen ());
}

int login_screen ()
{
  turn= 0;
  login_dialog[0].fg= gui_fg_color;
  login_dialog[0].bg= gui_bg_color;
  login_dialog[1].dp2= big_font;
  login_dialog[6].d1= player[0].type;
  login_dialog[7].d1= player[1].type;

  if (player[0].type==1) { login_dialog[4].flags|= D_DISABLED; }
  if (player[1].type==1) { login_dialog[5].flags|= D_DISABLED; }

  switch (do_dialog (login_dialog, -1))
  {
    case 8:
      set_rand_mine_field ();
      organize_mine_field ();
      obfuscate_mine_field ();
      clear_player_scores ();

      return (game_screen ());
      break;
    case 9:
    case 11:
      return (menu_screen ());
      break;
  }

  return (login_screen ());
}

int windowed_change ()
{
  if (windowed==0)
  {
    if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0)!=0)
    {
      if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0)!=0)
      {
        allegro_message ("Error starting up graphics: %s\n", allegro_error);
        allegro_exit ();
        return (-1);
      }
      init_theme ();
      alert ("Unable to set windowed mode:", allegro_error, "", "&Ok", NULL, 'o', 0);
      show_mouse (screen);
      windowed= 0;
      return (-1);
    }
  }
  else
  {
    if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0)!=0)
    {
      if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0)!=0)
      {
        allegro_message ("Error starting up graphics: %s\n", allegro_error);
        allegro_exit ();
        return (-1);
      }
      init_theme ();
      alert ("Unable to set fullscreen mode:", allegro_error, "", "&Ok", NULL, 'o', 0);
      show_mouse (screen);
      windowed= 1;
      return (-1);
    }
  }

  init_theme ();
  if (windowed==0) { windowed= 1; } else { windowed= 0; }
  return (0);
}

int menu_screen ()
{
  menu_dialog[0].fg= gui_fg_color;
  menu_dialog[0].bg= gui_bg_color;
  menu_dialog[1].dp2= big_font;

  switch (do_dialog (menu_dialog, -1))
  {
    case 2:
      return (login_screen ());
      break;
    case 3:
      return (instructions_screen ());
      break;
    case 4:
    case 8:
      set_config_int ("startup", "windowed", windowed);
      set_config_int ("startup", "sound", sound_status);
      set_config_string ("startup", "p1name", player[0].name);
      set_config_string ("startup", "p2name", player[1].name);
      set_config_int ("startup", "p1type", player[0].type);
      set_config_int ("startup", "p2type", player[1].type);
      allegro_exit ();
      exit (0);
      break;
    case 5:
      windowed_change ();
      break;
  }

  return (menu_screen ());
}

int main ()
{
  if (init ()==0)
  {
    menu_screen ();
  }

  return (0);
}
END_OF_MAIN ();
