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
#include <cstdint>

// Forward Declarations
class _Object;
class _Buffer;

// Classes
class _Component {

	public:

		_Component(_Object *Parent) : Parent(Parent), UpdateAutomatically(true) { }
		virtual ~_Component() { }

		// Updates
		virtual void Update(double FrameTime, uint16_t TimeSteps) { }

		// Network
		virtual void NetworkSerialize(_Buffer &Buffer) { }
		virtual void NetworkUnserialize(_Buffer &Buffer) { }
		virtual void NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) { }
		virtual void NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) { }

		// Attributes
		_Object *Parent;
		bool UpdateAutomatically;

};
