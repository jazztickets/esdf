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

// Libraries
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/detail/func_common.hpp>
#include <list>

// Forward Declarations
class _Texture;
class _Object;
class _Shot;
class _Shape;
struct _Impact;

// Holds data for a single tile
struct _Tile {
	_Tile() : TextureIndex(0) { }

	std::list<_Object *> Objects;
	uint32_t TextureIndex;
};

struct _Push {
	bool IsDiagonal() const { return Direction.x != 0 && Direction.y != 0; }
	_Object *Object;
	glm::vec2 Direction;
};

// Uniform grid class
class _Grid {

	public:

		_Grid();
		~_Grid();

		void InitTiles();

		// Objects
		void AddObject(_Object *Object);
		void RemoveObject(const _Object *Object);

		glm::ivec2 GetValidCoord(const glm::ivec2 &Coord) const { return glm::clamp(Coord, glm::ivec2(0), Size - 1); }
		void GetTileBounds(const _Object *Object, glm::ivec4 &Bounds) const;

		// Collision
		void CheckCollisions(const _Object *Object, std::list<_Push> &Pushes, bool &AxisAlignedPush) const;
		void ClampObject(_Object *Object) const;
		bool CanShootThrough(int IndexX, int IndexY) const { return true; }
		bool IsVisible(const glm::vec2 &Start, const glm::vec2 &End) const;
		void CheckBulletCollisions(const _Shot *Shot, _Impact &Impact) const;
		float RayObjectIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction, const _Object *Object) const;

		// Attributes
		glm::ivec2 Size;
		_Tile **Tiles;

	private:

};
