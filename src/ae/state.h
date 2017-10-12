/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#pragma once

#include <ae/input.h>

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
		virtual void WindowEvent(uint8_t Event) { }

		// Update
		virtual void Update(double FrameTime) { };
		virtual void Render(double BlendFactor) { };

	protected:

};
