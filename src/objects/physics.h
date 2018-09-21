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
#include <ae/circular_buffer.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <cstdint>
#include <unordered_map>

// Forward Declarations
struct _PhysicsStat;

// Classes
class _Physics : public _Component {

	public:

		struct _History {
			_History() { }
			_History(const glm::vec3 &Position, uint16_t Time) : Position(Position), Time(Time) { }

			glm::vec3 Position;
			uint16_t Time;
		};

		_Physics(_Object *Parent, const _PhysicsStat *Stats);
		~_Physics();

		// Updates
		void Update(double FrameTime) override;
		void ForcePosition(const glm::vec2 &Position);
		void FacePosition(const glm::vec2 &Cursor);

		// Network
		void NetworkSerialize(ae::_Buffer &Buffer) override;
		void NetworkUnserialize(ae::_Buffer &Buffer) override;
		void NetworkSerializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps) override;
		void NetworkUnserializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps) override;

		// Attributes
		std::unordered_map<_Object *, int> Touching;

		ae::_CircularBuffer<_History> History;
		glm::vec3 Position;
		glm::vec3 LastPosition;
		glm::vec3 NetworkPosition;
		glm::vec3 Velocity;
		float Rotation;
		float InterpolatedRotation;
		bool RenderDelay : 1;
		bool CollisionResponse : 1;
};
