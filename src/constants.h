/******************************************************************************
* esdf
* Copyright (C) 2017  Alan Witkowski
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
#include <string>

//     Config
const  int          DEFAULT_CONFIG_VERSION         =  2;
const  int          DEFAULT_SAVE_VERSION           =  1;
const  glm::ivec2   DEFAULT_WINDOW_SIZE            =  glm::ivec2(1440,900);
const  int          DEFAULT_FULLSCREEN             =  0;
const  int          DEFAULT_AUDIOENABLED           =  1;
const  int          DEFAULT_VSYNC                  =  1;
const  int          DEFAULT_ANISOTROPY             =  0;
const  double       DEFAULT_MAXFPS                 =  180.0;
const  size_t       DEFAULT_MAXCLIENTS             =  64;
const  double       DEFAULT_NETWORKRATE            =  1.0/20.0;
const  uint16_t     DEFAULT_NETWORKPORT            =  31234;
const  int          DEFAULT_KEYUP                  =  SDL_SCANCODE_E;
const  int          DEFAULT_KEYDOWN                =  SDL_SCANCODE_D;
const  int          DEFAULT_KEYLEFT                =  SDL_SCANCODE_S;
const  int          DEFAULT_KEYRIGHT               =  SDL_SCANCODE_F;
const  int          DEFAULT_KEYUSE                 =  SDL_SCANCODE_SPACE;
const  int          DEFAULT_KEYINVENTORY           =  SDL_SCANCODE_C;
const  int          DEFAULT_BUTTONFIRE             =  1;
//     Game
const  double       GAME_FPS                       =  100.0;
const  double       GAME_TIMESTEP                  =  1.0/GAME_FPS;
const  double       DEFAULT_AUTOSAVE_PERIOD        =  60.0;
const  double       MATH_PI                        =  3.14159265358979323846;
//     Camera
const  float        CAMERA_DISTANCE                =  6.5f;
const  float        CAMERA_DIVISOR                 =  15.0f;
const  float        CAMERA_EDITOR_DIVISOR          =  5.0f;
const  float        CAMERA_FOVY                    =  90.0f;
const  float        CAMERA_NEAR                    =  0.1f;
const  float        CAMERA_FAR                     =  100.0f;
//     Map
const  int          MAP_FILEVERSION                =  6;
const  std::string  MAP_DEFAULT_TILESET            =  "textures/tiles/atlas0.png";
const  float        MAP_WALLZ                      =  2.0f;
const  glm::ivec2   MAP_SIZE                       =  glm::ivec2(100,100);
const  float        MAP_BLOCK_ADJUST               =  0.001f;
//     Editor
const  std::string  EDITOR_TESTLEVEL               =  "test.map";
const  int          EDITOR_DEFAULT_GRIDMODE        =  5;
const  glm::ivec2   EDITOR_VIEWPORT_OFFSET         =  glm::ivec2(224,168);
const  int          EDITOR_PALETTE_SIZE            =  64;
const  float        EDITOR_ALIGN_DIVISOR           =  10.0f;
const  float        EDITOR_MIN_BLOCK_SIZE          =  0.1f;
