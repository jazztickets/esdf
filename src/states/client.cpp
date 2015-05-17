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
#include <states/client.h>
#include <states/editor.h>
#include <states/null.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/particle.h>
#include <objects/controller.h>
#include <objects/render.h>
#include <objects/physics.h>
#include <constants.h>
#include <framework.h>
#include <graphics.h>
#include <menu.h>
#include <camera.h>
#include <assets.h>
#include <hud.h>
#include <map.h>
#include <audio.h>
#include <config.h>
#include <actions.h>
#include <particles.h>
#include <buffer.h>
#include <server.h>
#include <packet.h>
#include <stats.h>
#include <iostream>
#include <sstream>

#include <font.h>
#include <program.h>
#include <glm/gtc/type_ptr.hpp>

_ClientState ClientState;

// Constructor
_ClientState::_ClientState() :
	Level(""),
	SaveFilename("test.save"),
	TestMode(true),
	FromEditor(false),
	RunServer(true),
	Stats(nullptr),
	Server(nullptr),
	HostAddress("127.0.0.1"),
	ConnectPort(DEFAULT_NETWORKPORT) {
}

// Load level and set up objects
void _ClientState::Init() {
	Player = nullptr;
	Controller = nullptr;
	Camera = nullptr;
	Particles = nullptr;
	HUD = nullptr;
	Map = nullptr;
	Network = nullptr;
	Server = nullptr;

	if(RunServer) {
		Server = new _Server(ConnectPort);
		Server->Stats = Stats;
		Server->StartThread();
	}

	Network = new _ClientNetwork();
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);
	Network->Connect(HostAddress.c_str(), ConnectPort);

	Graphics.ChangeViewport(Graphics.WindowSize);
	Graphics.ShowCursor(false);

	Actions.ResetState();
}

// Close map
void _ClientState::Close() {
	delete Camera;
	delete Particles;
	delete HUD;
	delete Map;
	delete Network;
	delete Server;
}

// Action handler
bool _ClientState::HandleAction(int InputType, int Action, int Value) {
	if(!Player || IsPaused())
		return false;

	if(Value) {
		if(1 /*!Player->Player->IsDying()*/) {
			switch(Action) {
				case _Actions::INVENTORY:
					HUD->SetInventoryOpen(!HUD->GetInventoryOpen());
					//Player->SetCrouching(false);
				break;
				case _Actions::USE:
					SendUse();
				break;
				/*
				case _Actions::FIRE:
					if(!HUD->GetInventoryOpen()) {
						if(Player->CanAttack() && !Player->HasAmmo())
							Audio.Play(new _AudioSource(Audio.GetBuffer(Player->GetSample(SAMPLE_EMPTY))), Player->Position);

						if(Player->GetFireRate() == FIRERATE_SEMI)
							Player->SetAttackRequested(true);
					}
				break;
				case _Actions::RELOAD:
					if(!HUD->IsDragging()) {
						if(Player->IsReloading())
							Player->CancelReloading();
						else
							Player->StartReloading();
					}
				break;
				case _Actions::WEAPONSWITCH:
					if(!HUD->IsDragging())
						Player->StartWeaponSwitch(INVENTORY_MAINHAND, INVENTORY_OFFHAND);
				break;
				case _Actions::MEDKIT:
					//Player->SetMedkitRequested(true);
				break;*/
			}
		}
		else {
			//if(Action == _Actions::USE)
			//	RestartFromDeath();
		}
	}

	return false;
}

// Key handler
void _ClientState::KeyEvent(const _KeyEvent &KeyEvent) {
	if(IsPaused()) {
		//if(Player)
		//	Player->Player->StopAudio();
		Menu.KeyEvent(KeyEvent);
		return;
	}

	if(KeyEvent.Pressed) {
		switch(KeyEvent.Key) {
			case SDL_SCANCODE_ESCAPE:
				if(Server)
					Server->StopServer();
				else
					Network->Disconnect();
			break;
			case SDL_SCANCODE_F1:
				Menu.InitInGame();
			break;
			case SDL_SCANCODE_GRAVE:
				Audio.Play(new _AudioSource(Audio.GetBuffer("player_hit0")), WorldCursor);
			break;
		}
	}
}

// Mouse handler
void _ClientState::MouseEvent(const _MouseEvent &MouseEvent) {
	if(HUD)
		HUD->MouseEvent(MouseEvent);

	if(IsPaused())
		Menu.MouseEvent(MouseEvent);
}

// Send use command
void _ClientState::SendUse() {
	if(!Player)
		return;

	_Buffer Buffer;
	Buffer.Write<char>(Packet::CLIENT_USE);
	Network->SendPacket(&Buffer);
}

// Update
void _ClientState::Update(double FrameTime) {
	Network->Update(FrameTime);

	// Get events
	_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case _NetworkEvent::CONNECT: {
				HandleConnect();
			} break;
			case _NetworkEvent::DISCONNECT:
				if(FromEditor)
					Framework.ChangeState(&EditorState);
				else
					Framework.SetDone(true);
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

	// Handle pauses
	if(IsPaused()) {
		Menu.Update(FrameTime);
	}

	// Update world cursor
	if(Camera)
		Camera->ConvertScreenToWorld(Input.GetMouse(), WorldCursor);

	// Process input
	if(Player && Map && Controller) {

		// Get first 4 bits of input state - movement
		_Controller::_Input Input(TimeSteps, Actions.GetState() & 0xF);
		if(0 && Player->ID == 1) {
			Input.ActionState = 4;
			if(TimeSteps & 32)
				Input.ActionState = 8;
		}
		if(IsPaused())
			Input.ActionState = 0;
		else
			Controller->HandleCursor(WorldCursor);

		// Update player
		Controller->HandleInput(Input);
		Controller->History.PushBack(Input);
		Player->Physics->Update(FrameTime, TimeSteps);
	}

	// Update objects
	if(Map)
		Map->Update(FrameTime, TimeSteps);

	// Update camera
	if(Camera && Player) {
		Camera->SetPosition(Player->Physics->Position);
		Camera->Update(FrameTime);
	}

	// Update the HUD
	if(HUD)
		HUD->Update(FrameTime, 0.0f /*Player->GetCrosshairRadius(WorldCursor)*/);

	/*
	// Update particles
	Particles->Update(FrameTime);

	// Update audio
	Audio.SetPosition(Player->Position);

	// Get zoom state
	if(Player->GetCrouching()) {
		if(Map->IsVisible(Player->Position, WorldCursor)) {
			Camera->UpdatePosition((WorldCursor - Player->Position) / Player->GetZoomScale());
		}
		else {
			glm::vec2 Direction = WorldCursor - Player->Position;

			glm::vec2 NewPosition;
			Map->CheckBulletCollisions(Player->Position, Direction, nullptr, &NewPosition, 0, false);
			Camera->UpdatePosition((NewPosition - Player->Position) / Player->GetZoomScale());
		}
		Camera->SetDistance(CAMERA_DISTANCE_AIMED);
	}
	else
		Camera->SetDistance(CAMERA_DISTANCE);

	// Get item at cursor
	PreviousCursorItem = CursorItem;
	CursorItem = static_cast<_Item *>(Map->CheckCollisionsInGrid(WorldCursor, 0.05f, GRID_ITEM, nullptr));
	if(CursorItem && CursorItem == PreviousCursorItem)
		CursorItemTimer += FrameTime;
	else
		CursorItemTimer = 0;

	// Set cursor item
	if(CursorItem && !HUD->GetCursorOverItem() && (HUD->GetInventoryOpen() || CursorItemTimer > HUD_CURSOR_ITEM_WAIT))
		HUD->SetCursorOverItem(CursorItem);
	*/

	// Send network updates to server
	if(Player && Network->IsConnected() && Controller && Controller->History.Size() > 0 /* && Network->NeedsUpdate()*/) {

		// TODO init once
		_Buffer Buffer(200);
		Buffer.Write<char>(Packet::CLIENT_INPUT);

		// Build packet
		Controller->NetworkSerializeHistory(Buffer);

		// Send packet to host
		Network->SendPacket(&Buffer, _Network::UNSEQUENCED, 1);

		// Reset timer
		Network->ResetUpdateTimer();
	}

	if(Player)
		TimeSteps++;
}

// Render the state
void _ClientState::Render(double BlendFactor) {
	if(!Map || !Player)
		return;

	glm::vec3 LightPosition(Player->Physics->Position, 1.0f);
	glm::vec3 LightAttenuation(0.4f, 0.3f, 0.2f);
	glm::vec4 AmbientLight(0.05f, 0.05f, 0.09f, 1.0f);

	Assets.Programs["pos_uv"]->LightAttenuation = LightAttenuation;
	Assets.Programs["pos_uv"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv"]->AmbientLight = AmbientLight;
	Assets.Programs["pos_uv_norm"]->LightAttenuation = LightAttenuation;
	Assets.Programs["pos_uv_norm"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLight;

	// Setup the viewing matrix
	Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw the floor
	Map->RenderFloors();

	// Enable VBOs
	//Graphics.SetProgram(Assets.Programs["pos_uv"]);
	//Graphics.SetVBO(VBO_QUAD);

	// Draw floor decals
	//Particles->Render(_Particles::FLOOR_DECAL);

	// Draw the walls
	Map->RenderWalls();

	// Draw the drops
	Map->RenderProps();

	// Draw objects
	Map->RenderObjects(BlendFactor);

	//Graphics.SetDepthMask(false);
	//Graphics.SetProgram(Assets.Programs["pos_uv"]);
	//Graphics.SetVBO(VBO_QUAD);

	// Draw wall decals
	//Particles->Render(_Particles::WALL_DECAL);

	// Draw particles
	//Graphics.EnableParticleBlending();
	//Particles->Render(_Particles::NORMAL);
	//Graphics.DisableParticleBlending();

	//Graphics.SetDepthMask(true);

	// Draw the crosshair
	if(1 /*!Player->Player->IsDying()*/) {
		Graphics.SetDepthTest(false);
		HUD->RenderCrosshair(WorldCursor);
	}

	// Setup OpenGL for drawing the HUD
	Graphics.Setup2D();
	/*
	glm::ivec2 Start(Camera->GetAABB()[0], Camera->GetAABB()[1]);
	glm::ivec2 End(Camera->GetAABB()[2], Camera->GetAABB()[3]);

	for(int X = Start.x; X < End.x; X++) {
		for(int Y = Start.y; Y < End.y; Y++) {
			if(X > 0 && Y > 0) {
				glm::ivec2 P;
				Camera->ConvertWorldToScreen(glm::vec2(X-0.5f, Y-0.5f), P);
				std::ostringstream Buffer;
				size_t Count = 0;
				std::list<_Event *> &Events = Map->GetEventList(glm::ivec2(X, Y));
				for(auto Event : Events) {
					if(Event->GetActive())
						Count++;

				}
				Buffer << Count << "/" << Events.size();
				Assets.GetFont("hud_tiny")->DrawText(Buffer.str(), P.X, P.Y);
				Buffer.str("");
			}
		}
	}*/

	HUD->Render();

	if(IsPaused() /*|| (Player->Player  && Player->Player->IsDead())*/) {
		Graphics.FadeScreen(GAME_PAUSE_FADEAMOUNT);
	}

	// Draw in-game menu
	if(IsPaused()) {
		Menu.Render();
	}
	/*else if(Player->Player && Player->Player->IsDead()) {
		Graphics.ShowCursor(1);
		HUD->RenderDeathScreen();
	}*/

	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	const _Font *Font = Assets.Fonts["hud_tiny"];
	int X = 200;
	int Y = 20;
	std::ostringstream Buffer;
	Buffer << Network->GetSentSpeed() / 1024.0f << "KB/s";
	Font->DrawText("Send", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
	Buffer.str("");
	Y += 15;

	Buffer << Network->GetReceiveSpeed() / 1024.0f << "KB/s";
	Font->DrawText("Receive", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
	Buffer.str("");
	Y += 15;

	Buffer << Network->GetRTT() << "ms";
	Font->DrawText("RTT", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
	Buffer.str("");
	Y += 15;

	if(Player && Controller) {
		Buffer << Player->Controller->History.Size();
		Font->DrawText("Inputs", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
		Buffer.str("");
		Y += 15;

		Buffer << TimeSteps;
		Font->DrawText("TimeStep", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
		Buffer.str("");
		Y += 15;
	}
	Graphics.SetDepthMask(true);
}

bool _ClientState::IsPaused() {
	return Menu.GetState() != _Menu::STATE_NONE;
}

// Handle packet from server
void _ClientState::HandlePacket(_Buffer &Buffer) {
	char PacketType = Buffer.Read<char>();

	switch(PacketType) {
		case Packet::MAP_INFO:
			HandleMapInfo(Buffer);
		break;
		case Packet::OBJECT_LIST:
			HandleObjectList(Buffer);
		break;
		case Packet::OBJECT_UPDATES:
			HandleObjectUpdates(Buffer);
		break;
		case Packet::OBJECT_CREATE:
			HandleObjectCreate(Buffer);
		break;
		case Packet::OBJECT_DELETE:
			HandleObjectDelete(Buffer);
		break;
		case Packet::INVENTORY_CREATE:
			HandleInventoryCreate(Buffer);
		break;
	}
}

// Handle connection to server
void _ClientState::HandleConnect() {
	CursorItem = nullptr;
	PreviousCursorItem = nullptr;
	HUD = nullptr;

	//Log << TimeSteps << " -- CONNECT" << std::endl;

	if(Level == "")
		Level = "mptest0.map";

	_Buffer Buffer;
	Buffer.Write<char>(Packet::CLIENT_JOIN);
	Buffer.WriteString(Level.c_str());
	Network->SendPacket(&Buffer);

	// Set checkpoint from editor
	//if(FromEditor)
	//	Player->SetCheckpointIndex(CheckpointIndex);

	// Check for level override
	//if(Level == "")
	//	Level = Player->GetMapIdentifier();

	// Initialize hud
	HUD = new _HUD();

	// Set up graphics
	Camera = new _Camera(glm::vec2(0), CAMERA_DISTANCE, CAMERA_DIVISOR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Particles = new _Particles();
	Particles->SetCamera(Camera);
}

// Load the map
void _ClientState::HandleMapInfo(_Buffer &Buffer) {
	uint8_t MapID = Buffer.Read<uint8_t>();
	std::string NewMap = Buffer.ReadString();
	delete Map;
	Map = new _Map(NewMap, Stats, MapID);

	Map->SetParticles(Particles);
	Map->SetCamera(Camera);
	Player = nullptr;
	Controller = nullptr;
	HUD->SetPlayer(nullptr);
}

// Handle a complete list of objects from a map
void _ClientState::HandleObjectList(_Buffer &Buffer) {

	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	TimeSteps = Buffer.Read<uint16_t>();
	uint16_t ClientID = Buffer.Read<uint16_t>();
	LastServerTimeSteps = TimeSteps - 1;

	// Clear out old objects
	Map->DeleteObjects();

	// Read object list
	uint16_t ObjectCount = Buffer.Read<uint16_t>();
	for(uint16_t i = 0; i < ObjectCount; i++) {
		std::string Identifier = Buffer.ReadString();
		uint16_t NetworkID = Buffer.Read<uint16_t>();

		// Create object
		_Object *Object = Stats->CreateObject(Identifier, false);
		Object->Map = Map;
		Object->ID = NetworkID;
		Object->NetworkUnserialize(Buffer);

		// Add to map
		Map->AddObject(Object);
		Map->AddObjectToGrid(Object);

		// Keep track of player id
		if(ClientID == NetworkID)
			Player = Object;
	}

	if(Player) {
		Controller = Player->Controller;
		Player->Log = Log;
		Player->Physics->Interpolate = false;
		Player->Physics->ClientSidePrediction = true;
		HUD->SetPlayer(Player);
		//Player->Player->HUD = HUD;
		Camera->ForcePosition(Player->Physics->Position);
	}
}

// Handle incremental updates from a map
void _ClientState::HandleObjectUpdates(_Buffer &Buffer) {

	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	// Discard out of order packets
	uint16_t ServerTimeSteps = Buffer.Read<uint16_t>();
	if(!_Network::MoreRecentAck(LastServerTimeSteps, ServerTimeSteps, uint16_t(-1)))
		return;

	// Update objects
	Map->UpdateObjectsFromBuffer(Buffer, ServerTimeSteps);
	if(Player && Controller)
		Player->Controller->ReplayInput();

	LastServerTimeSteps = ServerTimeSteps;
}

// Handle a create packet
void _ClientState::HandleObjectCreate(_Buffer &Buffer) {

	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	// Get object properties
	std::string Identifier = Buffer.ReadString();
	uint16_t ID = Buffer.Read<uint16_t>();

	// Create object
	_Object *Object = Stats->CreateObject(Identifier, false);
	Object->ID = ID;
	Object->Map = Map;
	Object->NetworkUnserialize(Buffer);

	// Add to map
	Map->AddObject(Object);
	Map->AddObjectToGrid(Object);

	Object->Log = Log;
}

// Handle a delete packet
void _ClientState::HandleObjectDelete(_Buffer &Buffer) {

	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	// Delete object by id
	uint16_t ID = Buffer.Read<uint16_t>();
	_Object *Object = Map->GetObjectByID(ID);
	if(Object)
		Object->Deleted = true;
}

// Handle an inventory item creation
void _ClientState::HandleInventoryCreate(_Buffer &Buffer) {
	/*
	int Slot = Buffer.Read<char>();
	char Type = Buffer.Read<char>();
	uint16_t Count = Buffer.Read<uint16_t>();
	std::string Identifier = Buffer.ReadString();

	// Create items
	_Item *Item;
	switch(Type) {
		case _Object::ITEM:
			//Item = Assets.CreateMiscItem(Identifier, Count, ZERO_VECTOR);
		break;
		case _Object::AMMO:
			//Identifier = Buffer.ReadString();
			//Inventory[Slot] = Assets.CreateAmmoItem(Identifier, Count, ZERO_VECTOR);
		break;
		case _Object::UPGRADE:
			//Identifier = Buffer.ReadString();
			//Inventory[Slot] = Assets.CreateUpgradeItem(Identifier, Count, ZERO_VECTOR);
		break;
		case _Object::WEAPON:
			//LoadWeapon(Buffer, Count, Slot);
		break;
		case _Object::ARMOR:
			//Identifier = Buffer.ReadString();
			//Inventory[Slot] = Assets.CreateArmor(Identifier, Count, ZERO_VECTOR);
		break;
	}

	Player->SetInventory(Slot, Item);
	*/
}
