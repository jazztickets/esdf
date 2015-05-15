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

#include <glm/vec2.hpp>

// Bounds struct
struct _Bounds {
	bool PointInside(const glm::ivec2 &Point) const {
		return Point.x >= Start.x && Point.y >= Start.y && Point.x < End.x && Point.y < End.y;
	}

	glm::ivec2 Start;
	glm::ivec2 End;
};

// Alignment struct
struct _Alignment {

	enum HorizontalAlignment {
		LEFT,
		CENTER,
		RIGHT,
	};

	enum VerticalAlignment {
		TOP,
		MIDDLE,
		BOTTOM,
		BASELINE,
	};

	_Alignment() { Horizontal = LEFT; Vertical = TOP; }
	_Alignment(int Horizontal, int Vertical) : Horizontal(Horizontal), Vertical(Vertical) { }

	int Horizontal, Vertical;
};

const _Alignment LEFT_TOP         = _Alignment(_Alignment::LEFT, _Alignment::TOP);
const _Alignment CENTER_MIDDLE    = _Alignment(_Alignment::CENTER, _Alignment::MIDDLE);
const _Alignment LEFT_BASELINE    = _Alignment(_Alignment::LEFT, _Alignment::BASELINE);
const _Alignment RIGHT_BASELINE   = _Alignment(_Alignment::RIGHT, _Alignment::BASELINE);
const _Alignment CENTER_BASELINE  = _Alignment(_Alignment::CENTER, _Alignment::BASELINE);
