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

#include <state.h>

// Null state
class _ConvertState : public _State {

	public:

		// Setup
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void TextEvent(const char *Text) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void SetParam1(const std::string &String) { Param1 = String; }

	protected:

		std::string Param1;
};

extern _ConvertState ConvertState;