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

// Libraries
#include <ae/ui/ui.h>
#include <string>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

struct _KeyEvent {
	_KeyEvent(int Key, bool Pressed) : Key(Key), Pressed(Pressed) { }
	int Key;
	bool Pressed;
};

struct _MouseEvent {
	_MouseEvent(const glm::ivec2 &Position, int Button, bool Pressed) : Position(Position), Button(Button), Pressed(Pressed) { }
	glm::ivec2 Position;
	int Button;
	bool Pressed;
};

// Classes
class _Input {

	public:

		enum InputType {
			KEYBOARD,
			MOUSE_BUTTON,
			MOUSE_AXIS,
			JOYSTICK_BUTTON,
			JOYSTICK_AXIS,
			INPUT_COUNT,
		};

		_Input();

		void Update(double FrameTime);

		int KeyDown(int Key) { return KeyState[Key]; }
		bool ModKeyDown(int Key);
		bool MouseDown(Uint32 Button);

		const glm::ivec2 &GetMouse() { return Mouse; }

		static const char *GetKeyName(int Key);
		static const std::string &GetMouseButtonName(Uint32 Button);

	private:

		// States
		const Uint8 *KeyState;
		Uint32 MouseState;
		glm::ivec2 Mouse;
};

extern _Input Input;
