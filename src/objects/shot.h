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

// Libraries
#include <glm/vec2.hpp>

// Forward Declarations
class _Object;

// Raycasting bullet class
class _Shot {

	public:

		// Constructor
		_Shot(_Object *Owner, const glm::vec2 &Position, const glm::vec2 &Offset, const glm::vec2 &Direction, int TargetFilter, int Damage)
		:	Owner(Owner),
			Position(Position),
			Offset(Offset),
			Direction(Direction),
			//Degrees(Direction.ToDegrees()),
			TargetFilter(TargetFilter),
			Damage(Damage) {
			Degrees = 0;
		}

		~_Shot() { }

		_Object *Owner;
		glm::vec2 Position;
		glm::vec2 Offset;
		glm::vec2 Direction;
		float Degrees;
		int TargetFilter;
		int Damage;

	private:

};
