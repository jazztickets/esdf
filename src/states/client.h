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
#include <ae/log.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <list>

// Forward Declarations
template<class T> class _Manager;
class _Font;
class _HUD;
class _Map;
class _Object;
class _Controller;
class _Item;
class _Camera;
class _ClientNetwork;
class _Server;
class _Buffer;
class _Stats;

// Play state
class _ClientState : public _State {

	public:

		// Setup
		_ClientState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, size_t Action, int Value) override;
		void HandleKey(const _KeyEvent &HandleKey) override;
		void HandleMouseButton(const _MouseEvent &HandleMouseButton) override;
		void HandleWindow(uint8_t Event) override;

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
		void SetLog(_LogFile *Log) { this->Log = Log; }

	protected:

		void HandlePacket(_Buffer &Data);
		void HandleConnect();
		void HandleMapInfo(_Buffer &Data);
		void HandleObjectList(_Buffer &Data);
		void HandleObjectUpdates(_Buffer &Data);
		void HandleObjectCreate(_Buffer &Data);
		void HandleObjectDelete(_Buffer &Data);
		void HandleUpdateHealth(_Buffer &Data);

		void SendAttack();
		void SendUse();

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
		_LogFile *Log;
		double CursorItemTimer;

		// Map
		_Map *Map;

		// Entities
		_Manager<_Object> *ObjectManager;
		_Object *Player;
		_Controller *Controller;

		// HUD
		_HUD *HUD;
		_Item *CursorItem;
		_Item *PreviousCursorItem;

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
