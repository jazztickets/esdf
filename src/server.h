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
#include <memory>
#include <thread>
#include <list>

// Forward Declarations
template<class T> class _Manager;
class _Object;
class _ServerNetwork;
class _Buffer;
class _Peer;
class _Map;
class _Stats;
struct _NetworkEvent;

// Server class
class _Server {

	public:

		_Server(uint16_t NetworkPort);
		~_Server();

		void Update(double FrameTime);
		void StartThread();
		void JoinThread();
		void StopServer();

		_Map *GetMap(const std::string &MapName);
		void ChangePlayerMap(const std::string &MapName, _Peer *Peer);

		// State
		bool Done;
		bool StartDisconnect;
		bool StartShutdown;
		uint16_t TimeSteps;
		double Time;
		_LogFile Log;

		// Stats
		const _Stats *Stats;

		// Network
		std::unique_ptr<_ServerNetwork> Network;

		// Objects
		_Manager<_Map> *MapManager;
		_Manager<_Object> *ObjectManager;

	private:

		void HandleConnect(_NetworkEvent &Event);
		void HandleDisconnect(_NetworkEvent &Event);
		void HandlePacket(_Buffer *Data, _Peer *Peer);
		void HandleClientJoin(_Buffer *Data, _Peer *Peer);
		void HandleClientInput(_Buffer *Data, _Peer *Peer);
		void HandleClientAttack(_Buffer *Data, _Peer *Peer);
		void HandleClientUse(_Buffer *Data, _Peer *Peer);

		// Threading
		std::thread *Thread;
};
