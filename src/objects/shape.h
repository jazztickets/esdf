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
#include <objects/component.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
struct _CollisionShapeStat;

// Classes
class _CollisionShape : public _Component {

	public:

		_CollisionShape(_Object *Parent, const _CollisionShapeStat *Stats);
		~_CollisionShape();

		// Network
		void NetworkSerialize(_Buffer &Buffer) override;
		void NetworkUnserialize(_Buffer &Buffer) override;

		bool IsAABB() const { return HalfWidth[1] != 0.0f; }
		glm::vec4 GetAABB(const glm::vec3 &Position);

		// Attributes
		glm::vec3 HalfWidth;
		uint32_t LastCollisionID;

};
