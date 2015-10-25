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
#include <objects/component.h>
#include <glm/vec2.hpp>

// Forward Declarations
struct _ShotStat;

// Raycasting bullet class
class _Shot : public _Component {

	public:

		// Constructor
		_Shot(_Object *Parent, const _ShotStat *Stats);
		~_Shot() { }

		// Network
		void NetworkSerialize(_Buffer &Buffer) override;
		void NetworkUnserialize(_Buffer &Buffer) override;

		// Updates
		void Update(double FrameTime, uint16_t TimeSteps) override;

		void CalcDirectionFromRotation();

		glm::vec2 Position;
		glm::vec2 EndPosition;
		glm::vec2 Offset;
		glm::vec2 Direction;
		float Rotation;
		int TargetFilter;
		int Damage;

};
