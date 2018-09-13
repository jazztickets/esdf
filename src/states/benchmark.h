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

#include <ae/state.h>

// Null state
class _BenchmarkState : public ae::_State {

	public:

		// Setup
		void Init() override;
		void Close() override;

		// Input
		void HandleKey(const ae::_KeyEvent &HandleKey) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void SetParam1(const std::string &String) { Param1 = String; }

	protected:

		std::string Param1;
};

extern _BenchmarkState BenchmarkState;
