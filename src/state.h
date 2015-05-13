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

#include <input.h>

// Game state class
class _State {

	public:

		// Setup
		virtual void Init() { };
		virtual void Close() { };

		// Input
		virtual bool HandleAction(int InputType, int Action, int Value) { return false; }
		virtual void KeyEvent(const _KeyEvent &KeyEvent) { };
		virtual void TextEvent(const char *Text) { };
		virtual void MouseEvent(const _MouseEvent &MouseEvent) { }
		virtual void MouseWheelEvent(int Direction) { }

		// Update
		virtual void Update(double FrameTime) { };
		virtual void Render(double BlendFactor) { };

	protected:

};
