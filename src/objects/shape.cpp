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
#include <objects/shape.h>
#include <stats.h>
#include <ae/buffer.h>

// Constructor
_CollisionShape::_CollisionShape(_Object *Parent, const _CollisionShapeStat *Stat) :
	_Component(Parent),
	HalfWidth(Stat->HalfWidth),
	LastCollisionID(-1) {
}

// Destructor
_CollisionShape::~_CollisionShape() {
}

// Serialize
void _CollisionShape::NetworkSerialize(ae::_Buffer &Buffer) {
	Buffer.Write<glm::vec3>(HalfWidth);
}

// Unserialize
void _CollisionShape::NetworkUnserialize(ae::_Buffer &Buffer) {
	HalfWidth = Buffer.Read<glm::vec3>();
}

// Get a min max aabb
glm::vec4 _CollisionShape::GetAABB(const glm::vec3 &Position) {

	if(IsAABB()) {
		return glm::vec4(
				Position.x - HalfWidth.x,
				Position.y - HalfWidth.y,
				Position.x + HalfWidth.x,
				Position.y + HalfWidth.y
			);
	}
	else {
		return glm::vec4(
				Position.x - HalfWidth.x,
				Position.y - HalfWidth.x,
				Position.x + HalfWidth.x,
				Position.y + HalfWidth.x
			);
	}
}
