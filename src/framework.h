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
#include <ae/log.h>
#include <cstdint>

// Forward Declarations
class _State;
class _Stats;
class _FrameLimit;
union SDL_Event;

// Manages SDL and game state
class _Framework {

	public:

		enum StateType {
			INIT,
			UPDATE,
			CLOSE
		};

		// Setup
		void Init(int ArgumentCount, char **Arguments);
		void Close();

		// Update functions
		void Update();
		void Render();

		bool GetDone() { return Done; }
		void SetDone(bool Done) { this->Done = Done; }

		_State *GetState() { return State; }
		void ChangeState(_State *RequestedState);

	private:

		int GlobalKeyHandler(const SDL_Event &Event);
		void LoadAssets(bool Server);

		// States
		_State *State, *RequestedState;
		bool Done;
		StateType FrameworkState;

		// Time
		_FrameLimit *FrameLimit;
		uint64_t Timer;
		double TimeStep;
		double TimeStepAccumulator;

		// Data
		_Stats *Stats;
		_LogFile Log;
};

extern _Framework Framework;
