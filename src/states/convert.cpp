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
#include <states/convert.h>
#include <framework.h>
#include <map.h>

_ConvertState ConvertState;

void _ConvertState::Init() {
	//_Map *Map = new _Map(Param1);
	//Map->Save(Param1);
	//Framework.SetDone(true);
}

void _ConvertState::Close() {
}

// Action handler
bool _ConvertState::HandleAction(int InputType, int Action, int Value) {

	return false;
}

// Key handler
void _ConvertState::KeyEvent(const _KeyEvent &KeyEvent) {
}

// Text handler
void _ConvertState::TextEvent(const char *Text) {
}

// Mouse handler
void _ConvertState::MouseEvent(const _MouseEvent &MouseEvent) {
}

// Update
void _ConvertState::Update(double FrameTime) {
}

// Render the state
void _ConvertState::Render(double BlendFactor) {
}
