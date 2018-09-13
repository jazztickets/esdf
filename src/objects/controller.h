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
#include <cstdint>

// Forward Declarations
struct _ControllerStat;

// Classes
class _Controller : public _Component {

	public:

		struct _Input {
			_Input() { }
			_Input(uint16_t Time, uint8_t ActionState) : Time(Time), ActionState(ActionState) { }

			uint16_t Time;
			uint8_t ActionState;
		};

		_Controller(_Object *Parent, const _ControllerStat *Stats);
		~_Controller();

		void NetworkSerializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps) override;
		void NetworkUnserializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps) override;

		void HandleInput(const _Input &Input, bool ReplayingInput=false);
		void HandleCursor(const glm::vec2 &Cursor);

		void ReplayInput();
		void NetworkSerializeHistory(ae::_Buffer &Buffer);

		const _ControllerStat *Stats;
		ae::_CircularBuffer<_Input> History;
		uint16_t LastInputTime;
};
