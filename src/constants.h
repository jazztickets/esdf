/******************************************************************************
* esdf
* Copyright (C) 2015  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

// Includes
#include <SDL_keycode.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>

//     Config
const  glm::ivec2   DEFAULT_WINDOW_SIZE            =  glm::ivec2(1024,768);
const  int          DEFAULT_FULLSCREEN             =  0;
const  int          DEFAULT_AUDIOENABLED           =  1;
const  int          DEFAULT_VSYNC                  =  1;
const  int          DEFAULT_ANISOTROPY             =  0;
const  double       DEFAULT_MAXFPS                 =  120.0;
const  double       DEFAULT_NETWORKRATE            =  1.0/20.0;
const  uint16_t     DEFAULT_NETWORKPORT            =  31234;
const  int          DEFAULT_KEYUP                  =  SDL_SCANCODE_E;
const  int          DEFAULT_KEYDOWN                =  SDL_SCANCODE_D;
const  int          DEFAULT_KEYLEFT                =  SDL_SCANCODE_S;
const  int          DEFAULT_KEYRIGHT               =  SDL_SCANCODE_F;
const  int          DEFAULT_KEYUSE                 =  SDL_SCANCODE_SPACE;
const  int          DEFAULT_KEYINVENTORY           =  SDL_SCANCODE_C;
const  int          DEFAULT_BUTTONFIRE             =  1;
const  int          DEFAULT_BUTTONAIM              =  3;
const  int          DEFAULT_KEYRELOAD              =  SDL_SCANCODE_R;
const  int          DEFAULT_KEYWEAPONSWITCH        =  SDL_SCANCODE_W;
const  int          DEFAULT_KEYMEDKIT              =  SDL_SCANCODE_Q;
//     Game
const  std::string  GAME_WINDOWTITLE               =  "esdf";
const  double       GAME_FPS                       =  60.0;
const  double       GAME_TIMESTEP                  =  1.0/GAME_FPS;
const  float        GAME_PAUSE_FADEAMOUNT          =  0.7f;
const  int          GAME_MAX_LEVEL                 =  100;
const  int          GAME_SKILLLEVELS               =  20;
//     Camera
const  float        CAMERA_DISTANCE                =  6.5f;
const  float        CAMERA_DIVISOR                 =  15.0f;
const  float        CAMERA_EDITOR_DIVISOR          =  5.0f;
const  float        CAMERA_FOVY                    =  90.0f;
const  float        CAMERA_NEAR                    =  0.1f;
const  float        CAMERA_FAR                     =  100.0f;
//     Graphics
const  int          GRAPHICS_CIRCLE_VERTICES       =  32;
//     Audio
const  float        MAX_AUDIO_DISTANCE             =  30.0f;
const  float        MAX_AUDIO_DISTANCE_SQUARED     =  MAX_AUDIO_DISTANCE*MAX_AUDIO_DISTANCE;
//     Entities
const  float        ENTITY_MOVESOUNDDELAYFACTOR    =  0.02625f;
const  int          ENTITY_MINDAMAGEPOINTS         =  1;
//     Player
const  int          PLAYER_SAVEVERSION             =  0;
const  std::string  PLAYER_DEFAULTNAME             =  "Jackson";
const  int          INVENTORY_BAGSIZE              =  16;
//     Items
const  float        ITEM_SCALE                     =  0.5f;
const  float        ITEM_Z                         =  0.05f;
//     Objects
const  float        OBJECT_Z                       =  0.3f;
//     Map
const  int          MAP_FILEVERSION                =  3;
const  std::string  MAP_DEFAULT_TILESET            =  "atlas0.png";
const  float        MAP_MINZ                       =  0.0f;
const  float        MAP_WALLZ                      =  2.0f;
const  glm::ivec2   MAP_SIZE                       =  glm::ivec2(100,100);
//     Editor
const  std::string  EDITOR_TESTLEVEL               =  "test.map";
const  float        EDITOR_OBJECTRADIUS            =  0.4f;
const  int          EDITOR_DEFAULT_LAYER           =  1;
const  int          EDITOR_DEFAULT_GRIDMODE        =  5;
const  glm::ivec2   EDITOR_VIEWPORT_OFFSET         =  glm::ivec2(224,168);
const  int          EDITOR_PALETTE_SELECTEDSIZE    =  32;
const  float        EDITOR_ALIGN_DIVISOR           =  10.0f;
const  float        EDITOR_MIN_BLOCK_SIZE          =  0.1f;
//     Menu
const  float        MENU_ACCEPTINPUT_FADE          =  0.7f;
const  double       MENU_DOUBLECLICK_TIME          =  0.250;
const  double       MENU_CURSOR_PERIOD             =  0.5;
//     HUD
const  double       HUD_ENTITYHEALTHDISPLAYPERIOD  =  5.0;
const  double       HUD_CURSOR_ITEM_WAIT           =  0.5;
const  float        HUD_CROSSHAIRDIVISOR           =  5.0f;
const  float        HUD_MINCROSSHAIRSCALE          =  0.0f;
const  double       HUD_KEYMESSAGETIME             =  3.0;
const  double       HUD_CHECKPOINTTIME             =  5.0;
const  std::string  HUD_CHECKPOINTMESSAGE          =  "CHECKPOINT REACHED";
const  double       HUD_INVENTORYFULLTIME          =  2.0;
const  std::string  HUD_INVENTORYFULLMESSAGE       =  "INVENTORY FULL";
const  double       HUD_KEYUSEDTIME                =  2.0;
const  std::string  HUD_KEYUSEDMESSAGE             =  "KEY USED";
//     Textures
const  std::string  TEXTURES_PATH                  =  "textures/";
const  std::string  TEXTURES_ANIMATIONS            =  "animations/";
const  std::string  TEXTURES_BLOCKS                =  "blocks/";
const  std::string  TEXTURES_EDITOR                =  "editor/";
const  std::string  TEXTURES_EDITOR_REPEAT         =  "editor_repeat/";
const  std::string  TEXTURES_HUD                   =  "hud/";
const  std::string  TEXTURES_HUD_REPEAT            =  "hud_repeat/";
const  std::string  TEXTURES_MENU                  =  "menu/";
const  std::string  TEXTURES_PROPS                 =  "props/";
const  std::string  TEXTURES_TILES                 =  "tiles/";
//     Meshes
const  std::string  MESHES_PATH                    =  "meshes/";
const  std::string  MESHES_SUFFIX                  =  ".mesh";
//     Assets
const  std::string  ASSETS_FONTS                   =  "fonts/";
const  std::string  ASSETS_MAPS                    =  "maps/";
const  std::string  ASSETS_PROGRAMS                =  "tables/programs.tsv";
const  std::string  ASSETS_ANIMATIONS              =  "tables/animations.tsv";
const  std::string  ASSETS_COLORS                  =  "tables/colors.tsv";
const  std::string  ASSETS_FONT_TABLE              =  "tables/fonts.tsv";
const  std::string  ASSETS_STRINGS                 =  "tables/strings.tsv";
const  std::string  ASSETS_UI_BUTTONS              =  "tables/ui/buttons.tsv";
const  std::string  ASSETS_UI_ELEMENTS             =  "tables/ui/elements.tsv";
const  std::string  ASSETS_UI_IMAGES               =  "tables/ui/images.tsv";
const  std::string  ASSETS_UI_LABELS               =  "tables/ui/labels.tsv";
const  std::string  ASSETS_UI_STYLES               =  "tables/ui/styles.tsv";
const  std::string  ASSETS_UI_TEXTBOXES            =  "tables/ui/textboxes.tsv";
//     Stats
const  std::string  STATS_OBJECTS                  =  "stats/objects.tsv";
const  std::string  STATS_PHYSICS                  =  "stats/physics.tsv";
const  std::string  STATS_CONTROLLERS              =  "stats/controllers.tsv";
const  std::string  STATS_ANIMATIONS               =  "stats/animations.tsv";
const  std::string  STATS_RENDERS                  =  "stats/renders.tsv";
const  std::string  STATS_SHAPES                   =  "stats/shapes.tsv";
//     Scripts
const  std::string  SCRIPTS_PATH                   =  "scripts/";
const  std::string  SCRIPTS_DEFAULT                =  "default.lua";
//     Colors
const  glm::vec4    COLOR_WHITE                    =  {1.0f,1.0f,1.0f,1.0f};
const  glm::vec4    COLOR_TWHITE                   =  {1.0f,1.0f,1.0f,0.5f};
const  glm::vec4    COLOR_DARK                     =  {0.3f,0.3f,0.3f,1.0f};
const  glm::vec4    COLOR_TGRAY                    =  {1.0f,1.0f,1.0f,0.2f};
const  glm::vec4    COLOR_RED                      =  {1.0f,0.0f,0.0f,1.0f};
const  glm::vec4    COLOR_GREEN                    =  {0.0f,1.0f,0.0f,1.0f};
const  glm::vec4    COLOR_BLUE                     =  {0.0f,0.0f,1.0f,1.0f};
const  glm::vec4    COLOR_YELLOW                   =  {1.0f,1.0f,0.0f,1.0f};
const  glm::vec4    COLOR_MAGENTA                  =  {1.0f,0.0f,1.0f,1.0f};
const  glm::vec4    COLOR_CYAN                     =  {0.0f,1.0f,1.0f,1.0f};
