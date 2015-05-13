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
#include <circular_buffer.h>
#include <cstdint>

// Forward Declarations
class _Object;
class _Buffer;

// Classes
class _Physics {

	public:

		struct _History {
			_History() { }
			_History(const glm::vec2 &Position, uint16_t Time) : Position(Position), Time(Time) { }

			glm::vec2 Position;
			uint16_t Time;
		};

		_Physics(_Object *Parent);
		~_Physics();

		// Network
		void NetworkSerialize(_Buffer &Buffer);
		void NetworkUnserialize(_Buffer &Buffer);
		void NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps);
		void NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps);

		// Updates
		void ForcePosition(const glm::vec2 &Position);
		void FacePosition(const glm::vec2 &Cursor);
		void Update(double FrameTime, uint16_t TimeSteps);

		// Attributes
		_Object *Parent;

		_CircularBuffer<_History> History;
		glm::vec2 Position;
		glm::vec2 LastPosition;
		glm::vec2 NetworkPosition;
		glm::vec2 Velocity;
		float Rotation;
		float InterpolatedRotation;
		float Radius;
		int Interpolate : 1;
		int ClientSidePrediction : 1;
};
