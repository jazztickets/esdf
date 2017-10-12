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
#include <ae/input.h>
#include <list>
#include <string>
#include <SDL_scancode.h>

// Constants
const int ACTIONS_MAXINPUTS = SDL_NUM_SCANCODES;

// Actions class
class _Actions {

	public:

		enum Types {
			UP,
			DOWN,
			LEFT,
			RIGHT,
			FIRE,
			AIM,
			USE,
			COUNT,
		};

		_Actions();

		void ResetState() { State = 0; }
		void ClearMappings(int InputType);
		void ClearMappingsForAction(int InputType, int Action);
		void ClearAllMappingsForAction(int Action);

		// Actions
		int GetState(int Action);
		int GetState() { return State; }
		const std::string &GetName(int Action) { return Names[Action]; }

		// Maps
		void AddInputMap(int InputType, int Input, int Action, bool IfNone=true);
		int GetInputForAction(int InputType, int Action);
		std::string GetInputNameForAction(int Action);

		// Handlers
		void InputEvent(int InputType, int Input, int Value);

	private:

		// Input bindings
		std::list<int> InputMap[_Input::INPUT_COUNT][ACTIONS_MAXINPUTS];

		// State of each action
		int State;

		// Nice names for each action
		std::string Names[COUNT];
};

extern _Actions Actions;
