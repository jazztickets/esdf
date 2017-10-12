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

#include <cstddef>

namespace Action {
	enum size_t {
		GAME_UP,
		GAME_DOWN,
		GAME_LEFT,
		GAME_RIGHT,
		GAME_USE,
		GAME_FIRE,
		MENU_UP,
		MENU_DOWN,
		MENU_LEFT,
		MENU_RIGHT,
		MENU_GO,
		MENU_BACK,
		MENU_PAUSE,
		MISC_CONSOLE,
		MISC_DEBUG,
		COUNT,
	};
}