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
class _HUD;
class _Map;
class _Object;
class _Controller;
class _Item;
class _Server;
class _Stats;

namespace ae {
	template<class T> class _Manager;
	class _ClientNetwork;
	class _Camera;
	class _Font;
	class _Buffer;
}

// Play state
class _ClientState : public ae::_State {

	public:

		// Setup
		_ClientState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, size_t Action, int Value) override;
		bool HandleKey(const ae::_KeyEvent &HandleKey) override;
		void HandleMouseButton(const ae::_MouseEvent &HandleMouseButton) override;
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
		void SetLog(ae::_LogFile *Log) { this->Log = Log; }

		void StopLocalServer();

	protected:

		void HandlePacket(ae::_Buffer &Data);
		void HandleConnect();
		void HandleMapInfo(ae::_Buffer &Data);
		void HandleObjectList(ae::_Buffer &Data);
		void HandleObjectUpdates(ae::_Buffer &Data);
		void HandleObjectCreate(ae::_Buffer &Data);
		void HandleObjectDelete(ae::_Buffer &Data);
		void HandleUpdateHealth(ae::_Buffer &Data);

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
		ae::_LogFile *Log;

		// Map
		_Map *Map;

		// Entities
		ae::_Manager<_Object> *ObjectManager;
		_Object *Player;
		_Controller *Controller;

		// HUD
		_HUD *HUD;

		// Camera
		ae::_Camera *Camera;
		glm::vec2 WorldCursor;

		// Network
		ae::_ClientNetwork *Network;
		_Server *Server;
		std::string HostAddress;
		uint16_t TimeSteps;
		uint16_t LastServerTimeSteps;
		uint16_t ConnectPort;

};

extern _ClientState ClientState;
