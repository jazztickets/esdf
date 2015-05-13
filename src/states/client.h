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
#include <log.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <list>

// Forward Declarations
class _Font;
class _HUD;
class _Map;
class _Object;
class _Controller;
class _Item;
class _Particles;
class _Camera;
class _ClientNetwork;
class _Server;
class _Buffer;
class _Stats;
struct _Spawn;

// Play state
class _ClientState : public _State {

	public:

		// Setup
		_ClientState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;
		void SendUse();

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// State parameters
		void SetLevel(const std::string &Level) { this->Level = Level; }
		void SetTestMode(bool Value) { TestMode = Value; }
		void SetFromEditor(bool Value) { FromEditor = Value; }
		void SetCheckpointIndex(int Value) { CheckpointIndex = Value; }
		void SetStats(const _Stats *Stats)  { this->Stats = Stats; }
		bool GetFromEditor() const { return FromEditor; }

		void SetSaveFilename(const std::string &SaveFilename) { this->SaveFilename = SaveFilename; }
		void SetHostAddress(const std::string &HostAddress) { this->HostAddress = HostAddress; }
		void SetConnectPort(uint16_t ConnectPort) { this->ConnectPort = ConnectPort; }
		void SetRunServer(bool RunServer) { this->RunServer = RunServer; }
		void SetLog(_Log *Log) { this->Log = Log; }

	protected:

		void HandlePacket(_Buffer &Buffer);
		void HandleConnect();
		void HandleMapInfo(_Buffer &Buffer);
		void HandleObjectList(_Buffer &Buffer);
		void HandleObjectUpdates(_Buffer &Buffer);
		void HandleObjectCreate(_Buffer &Buffer);
		void HandleObjectDelete(_Buffer &Buffer);
		void HandleInventoryCreate(_Buffer &Buffer);

		bool IsPaused();

		// Parameters
		std::string Level;
		std::string SaveFilename;
		bool TestMode;
		bool FromEditor;
		bool RunServer;
		int CheckpointIndex;

		// Game
		const _Stats *Stats;
		_Log *Log;
		double CursorItemTimer;

		// Map
		_Map *Map;

		// Entities
		_Object *Player;
		_Controller *Controller;

		// HUD
		_HUD *HUD;
		_Item *CursorItem;
		_Item *PreviousCursorItem;

		// Particles
		_Particles *Particles;

		// Camera
		_Camera *Camera;
		glm::vec2 WorldCursor;

		// Network
		_ClientNetwork *Network;
		_Server *Server;
		std::string HostAddress;
		uint16_t TimeSteps;
		uint16_t LastServerTimeSteps;
		uint16_t ConnectPort;

};

extern _ClientState ClientState;