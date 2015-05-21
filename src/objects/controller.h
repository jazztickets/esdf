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
#include <circular_buffer.h>
#include <glm/vec2.hpp>
#include <cstdint>

// Forward Declarations
class _Object;
class _Buffer;
struct _ControllerStat;

// Classes
class _Controller {

	public:

		struct _Input {
			_Input() { }
			_Input(uint16_t Time, uint8_t ActionState) : Time(Time), ActionState(ActionState) { }

			uint16_t Time;
			uint8_t ActionState;
		};

		_Controller(_Object *Parent, const _ControllerStat &Stat);
		~_Controller();

		void NetworkSerialize(_Buffer &Buffer);
		void NetworkUnserialize(_Buffer &Buffer);
		void NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps);
		void NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps);

		void HandleInput(const _Input &Input, bool ReplayingInput=false);
		void HandleCursor(const glm::vec2 &Cursor);

		void ReplayInput();
		void NetworkSerializeHistory(_Buffer &Buffer);

		_Object *Parent;
		const _ControllerStat &Stat;
		_CircularBuffer<_Input> History;
		uint16_t LastInputTime;
};
