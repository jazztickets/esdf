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
#include <ae/actions.h>
#include <framework.h>
#include <ae/state.h>
#include <ae/assets.h>
#include <ae/ui/label.h>

_Actions Actions;

// Constructor
_Actions::_Actions() {
	ResetState();
}

// Clear all mappings
void _Actions::ClearMappings(int InputType) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++)
		InputMap[InputType][i].clear();
}

// Remove a mapping for an action
void _Actions::ClearMappingsForAction(int InputType, int Action) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++) {
		for(auto MapIterator = InputMap[InputType][i].begin(); MapIterator != InputMap[InputType][i].end(); ) {
			if(*MapIterator == Action) {
				MapIterator = InputMap[InputType][i].erase(MapIterator);
			}
			else
				++MapIterator;
		}
	}
}

// Remove all input mappings for an action
void _Actions::ClearAllMappingsForAction(int Action) {
	for(int i = 0; i < _Input::INPUT_COUNT; i++) {
		ClearMappingsForAction(i, Action);
	}
}

// Get action
int _Actions::GetState(int Action) {
	return !!(State & (1 << Action));
}

// Add an input mapping
void _Actions::AddInputMap(int InputType, int Input, int Action, bool IfNone) {
	if(Action < 0 || Action >= COUNT || Input < 0 || Input >= ACTIONS_MAXINPUTS)
		return;

	if(!IfNone || (IfNone && GetInputForAction(InputType, Action) == -1)) {
		InputMap[InputType][Input].push_back(Action);
	}
}

// Returns the first input for an action
int _Actions::GetInputForAction(int InputType, int Action) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++) {
		for(auto &Iterator : InputMap[InputType][i]) {
			if(Iterator == Action) {
				return i;
			}
		}
	}

	return -1;
}

// Get name of input key/button for a given action
std::string _Actions::GetInputNameForAction(int Action) {

	for(int i = 0; i < _Input::INPUT_COUNT; i++) {
		int Input = GetInputForAction(i, Action);

		if(Input != -1) {
			switch(i) {
				case _Input::KEYBOARD:
					return _Input::GetKeyName(Input);
				break;
				case _Input::MOUSE_BUTTON:
					return _Input::GetMouseButtonName(Input);
				break;
			}
		}
	}

	return "";
}

// Inject an input into the action handler
void _Actions::InputEvent(int InputType, int Input, int Value) {
	if(Input < 0 || Input >= ACTIONS_MAXINPUTS)
		return;

	for(auto &Action : InputMap[InputType][Input]) {
		State &= ~(1 << Action);
		State |= Value << Action;

		// If true is returned, stop handling the same key
		if(Framework.GetState()->HandleAction(InputType, Action, Value))
			break;
	}
}
