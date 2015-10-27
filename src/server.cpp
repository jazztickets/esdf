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
#include <server.h>
#include <network/servernetwork.h>
#include <network/peer.h>
#include <objects/object.h>
#include <objects/controller.h>
#include <objects/physics.h>
#include <objects/shot.h>
#include <scripting.h>
#include <buffer.h>
#include <packet.h>
#include <map.h>
#include <grid.h>
#include <stats.h>
#include <constants.h>
#include <config.h>
#include <SDL_timer.h>

// Function to run the server thread
void RunThread(void *Arguments) {

	// Get server object
	_Server *Server = (_Server *)Arguments;

	// Init timer
	Uint64 Timer = SDL_GetPerformanceCounter();
	double TimeStepAccumulator = 0.0;
	double TimeStep = GAME_TIMESTEP;
	while(!Server->Done) {
		double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
		Timer = SDL_GetPerformanceCounter();

		// Run server
		TimeStepAccumulator += FrameTime;
		while(TimeStepAccumulator >= TimeStep) {
			Server->Update(TimeStep);
			TimeStepAccumulator -= TimeStep;
		}

		// Sleep thread
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// Constructor
_Server::_Server(uint16_t NetworkPort)
:	Done(false),
	StartShutdown(false),
	TimeSteps(0),
	Time(0.0),
	Stats(nullptr),
	Network(new _ServerNetwork(NetworkPort)),
	NextMapID(0),
	Thread(nullptr) {

	if(!Network->HasConnection())
		throw std::runtime_error("Unable to bind address!");

	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);
	Log.Open((Config.ConfigPath + "server.log").c_str());
	//Log.SetToStdOut(true);
}

// Destructor
_Server::~_Server() {
	Done = true;
	if(Thread)
		Thread->join();

	// Delete maps
	for(auto &Map : Maps) {
		delete Map;
	}

	delete Thread;
}

// Start the server thread
void _Server::StartThread() {
	Thread = new std::thread(RunThread, this);
}

// Stop the server
void _Server::StopServer() {
	Network->DisconnectAll();
	StartShutdown = true;
}

// Update
void _Server::Update(double FrameTime) {
	//Log << "ServerUpdate " << TimeSteps << std::endl;

	// Update network
	Network->Update(FrameTime);

	// Get events
	_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case _NetworkEvent::CONNECT:
				HandleConnect(NetworkEvent);
			break;
			case _NetworkEvent::DISCONNECT:
				HandleDisconnect(NetworkEvent);
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(NetworkEvent.Data, NetworkEvent.Peer);
				delete NetworkEvent.Data;
			break;
		}
	}

	// Run player inputs
	auto &Peers = Network->GetPeers();
	for(auto &Peer : Peers) {
		_Object *Player = Peer->Object;
		if(Player && Player->HasComponent("controller")) {
			_Controller *Controller = (_Controller *)Player->Components["controller"];

			auto &InputHistory = Controller->History;
			if(InputHistory.IsEmpty()) {
				//Log << "StarvedInput= " << Player->GetID() << std::endl
				break;
			}

			int InputsToPlay = 1;
			if(InputHistory.Size() > 3)
				InputsToPlay = 3;

			while(InputsToPlay) {
				auto &InputState = InputHistory.Front();
				Controller->HandleInput(InputState);
				Player->Physics->Update(FrameTime, InputState.Time);
				Player->SendUpdate = true;
				Controller->LastInputTime = InputState.Time;
				//Player->Map->CheckEvents(Player, this);
				InputHistory.Pop();
				InputsToPlay--;

				//Log << "PlayerInputCount= " << InputHistory.Size() << std::endl;
			}
		}
	}

	// Update maps
	for(auto &Map : Maps)
		Map->Update(FrameTime, TimeSteps);

	// Check if updates should be sent
	if(Network->NeedsUpdate()) {
		//Log << "NeedsUpdate " << TimeSteps << std::endl;
		Network->ResetUpdateTimer();
		if(0 && (rand() % 10) == 0) {
			//printf("droppin pack\n");
		}
		else if(Network->GetPeers().size() > 0) {

			// Notify
			for(auto &Map : Maps) {
				_Buffer Buffer;
				Buffer.Write<char>(Packet::OBJECT_UPDATES);
				Buffer.Write<uint8_t>(Map->ID);
				Buffer.Write<uint16_t>(TimeSteps);
				Map->BuildObjectUpdate(Buffer, TimeSteps);

				const std::list<const _Peer *> Peers = Map->GetPeers();
				for(auto &Peer : Peers) {
					Network->SendPacket(Buffer, Peer, _Network::UNSEQUENCED, 1);
				}
			}
		}
	}

	if(StartShutdown && Network->GetPeers().size() == 0) {
		Done = true;
	}

	TimeSteps++;
	Time += FrameTime;
}

// Handle client connect
void _Server::HandleConnect(_NetworkEvent &Event) {
	//Log << TimeSteps << " -- connect peer_count=" << (int)Network->GetPeers().size() << std::endl;
}

// Handle client disconnect
void _Server::HandleDisconnect(_NetworkEvent &Event) {

	// Get object
	_Object *Object = Event.Peer->Object;
	if(!Object)
		return;

	// Update map
	_Map *Map = Object->Map;
	if(Map) {
		Map->RemovePeer(Event.Peer);
	}

	// Remove from list
	Object->Deleted = true;

	// Delete peer from network
	Network->DeletePeer(Event.Peer);
}

// Handle packet data
void _Server::HandlePacket(_Buffer *Data, _Peer *Peer) {
	char PacketType = Data->Read<char>();

	switch(PacketType) {
		case Packet::CLIENT_JOIN:
			HandleClientJoin(Data, Peer);
		break;
		case Packet::CLIENT_INPUT:
			HandleClientInput(Data, Peer);
		break;
		case Packet::CLIENT_ATTACK:
			HandleClientAttack(Data, Peer);
		break;
		case Packet::CLIENT_USE:
			HandleClientUse(Data, Peer);
		break;
	}
}

// Handle a client joining the game
void _Server::HandleClientJoin(_Buffer *Data, _Peer *Peer) {

	// Make sure the peer doesn't already have a player object
	if(Peer->Object)
		return;

	// Get map
	std::string MapFilename = Data->ReadString();
	ChangePlayerMap(MapFilename, Peer);
}

// Handle input from client
void _Server::HandleClientInput(_Buffer *Data, _Peer *Peer) {

	// Get player object
	_Object *Object = Peer->Object;
	if(!Object)
		return;

	_Controller *Controller = (_Controller *)Object->Components["controller"];
	//if((rand() % 5) == 0) return;

	// Read packet
	Object->Physics->Rotation = Data->Read<float>();

	_Controller::_Input InputState;
	InputState.Time = Data->Read<uint16_t>();

	// Apply player movement
	//Log << "New pack" << std::endl;
	while(!Data->End()) {
		uint8_t PacketByte = Data->Read<uint8_t>();

		// Process packet
		int KeyCount = (PacketByte >> 4) + 1;
		InputState.ActionState = 15 & PacketByte;

		// Apply input
		for(int i = 0; i < KeyCount; i++) {
			if(_Network::MoreRecentAck(Peer->LastAck, InputState.Time, uint16_t(-1))) {
				Controller->History.PushBack(InputState);
				Peer->LastAck = InputState.Time;
				//Log << "PlayerInput= " << Player->GetID() << " Server.Time= " << TimeSteps << " InputState.Time= " << InputState.Time << std::endl;
			}

			InputState.Time++;
		}
	}

	//Log << "PosAfterInput= " << Player->GetID() << " Server.Time= " << TimeSteps << " LastAck= " << Peer->GetLastAck() << " X= " << Player->Position.X << std::endl;
}

// Client attack command
void _Server::HandleClientAttack(_Buffer *Data, _Peer *Peer) {

	// Get player object
	_Object *Player = Peer->Object;
	if(!Player)
		return;

	// Get attack info
	float Rotation = Data->Read<float>();

	// Create new shot
	_Object *Object = Stats->CreateObject("shot", true);
	_Map *Map = Player->Map;

	Object->Parent = Player;
	Object->ID = Map->GenerateObjectID();
	Object->Map = Map;
	if(Object->HasComponent("shot")) {
		_Shot *Shot = (_Shot *)(Object->Components["shot"]);
		Shot->Position = glm::vec2(Player->Physics->Position);
		Shot->Rotation = Rotation;
		Shot->CalcDirectionFromRotation();
	}

	//Object->Deleted = true;
	Map->AddObject(Object);

	// Create object create packet
	{
		_Buffer Buffer;
		Buffer.Write<char>(Packet::OBJECT_CREATE);
		Buffer.Write<uint8_t>(Map->ID);
		Object->NetworkSerialize(Buffer);

		// Broadcast to all other peers
		Map->BroadcastPacket(Buffer);
	}
}

// Client use command
void _Server::HandleClientUse(_Buffer *Data, _Peer *Peer) {

	// Get player object
	_Object *Player = Peer->Object;
	if(!Player)
		return;

	/*_Buffer Buffer;
	Player->StartUse(Buffer);
	if(Buffer.GetCurrentSize() > 0) {
		Network->SendPacket(Buffer, Peer);
	}*/
}

// Send map information to a client
void _Server::ChangePlayerMap(const std::string &MapName, _Peer *Peer) {
	_Map *Map = GetMap(MapName);
	if(!Map)
		return;

	// Delete old player
	_Object *OldPlayer = Peer->Object;
	if(OldPlayer)
		OldPlayer->Deleted = true;

	// Create new player
	_Object *Object = Stats->CreateObject("player", true);
	Object->ID = Map->GenerateObjectID();
	Object->Map = Map;
	Object->Physics->RenderDelay = false;
	if(OldPlayer)
		Object->Physics->Rotation = OldPlayer->Physics->Rotation;
	Object->Physics->ForcePosition(Map->GetStartingPositionByCheckpoint(0));
	Object->Physics->UpdateAutomatically = false;
	Object->Peer = Peer;
	Map->AddObject(Object);
	Map->Grid->AddObject(Object);

	// Create object create packet
	{
		_Buffer Buffer;
		Buffer.Write<char>(Packet::OBJECT_CREATE);
		Buffer.Write<uint8_t>(Map->ID);
		Object->NetworkSerialize(Buffer);

		// Broadcast to all other peers
		Map->BroadcastPacket(Buffer);
	}

	Peer->Object = Object;
	Peer->LastAck = TimeSteps;

	// Send map name
	_Buffer Buffer;
	Buffer.Write<char>(Packet::MAP_INFO);
	Buffer.Write<uint8_t>(Map->ID);
	Buffer.WriteString(MapName.c_str());
	Network->SendPacket(Buffer, Peer);

	// Add peer to map
	Map->AddPeer(Peer);
	Map->SendObjectList(Object, TimeSteps);
}

// Get a map if it's already loaded, if not load it and return it
_Map *_Server::GetMap(const std::string &MapName) {

	// Search for loaded map
	for(auto &Map : Maps) {
		if(Map->Filename == MapName) {
			return Map;
		}
	}

	// Load map
	_Map *Map = nullptr;
	try {
		//TODO fix NextMapID
		Map = new _Map(MapName, Stats, true, NextMapID++, Network.get());
		Map->Scripting->Server = this;
	}
	catch(std::exception &Error) {
		Log << TimeSteps << " -- Error loading map: " << MapName << std::endl;
	}

	// Add to list of maps
	if(Map)
		Maps.push_back(Map);

	return Map;
}
