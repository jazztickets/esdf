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
#include <states/client.h>
#include <states/editor.h>
#include <states/null.h>
#include <ae/clientnetwork.h>
#include <objects/object.h>
#include <objects/controller.h>
#include <objects/render.h>
#include <objects/physics.h>
#include <objects/shot.h>
#include <objects/health.h>
#include <constants.h>
#include <framework.h>
#include <ae/graphics.h>
#include <menu.h>
#include <ae/camera.h>
#include <ae/assets.h>
#include <hud.h>
#include <map.h>
#include <grid.h>
#include <ae/audio.h>
#include <config.h>
#include <ae/actions.h>
#include <ae/buffer.h>
#include <server.h>
#include <packet.h>
#include <stats.h>
#include <actiontype.h>
#include <iostream>
#include <sstream>

#include <ae/font.h>
#include <ae/program.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

	Graphics.ChangeViewport(Graphics.CurrentSize);
	Graphics.ShowCursor(CURSOR_CROSS);

	Actions.ResetState();
}

// Close map
void _ClientState::Close() {
	delete Camera;
	delete HUD;
	delete Map;
	delete Network;
	delete Server;
}

// Action handler
bool _ClientState::HandleAction(int InputType, size_t Action, int Value) {
	if(!Player || IsPaused())
		return false;

	if(Value) {
		switch(Action) {
			case Action::GAME_FIRE:
				SendAttack();
			break;
			case Action::GAME_USE:
				SendUse();
			break;
		}
	}

	return false;
}

// Key handler
void _ClientState::HandleKey(const _KeyEvent &KeyEvent) {
	if(IsPaused()) {
		Menu.KeyEvent(KeyEvent);
		return;
	}

	if(KeyEvent.Pressed) {
		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_ESCAPE:
				if(Server)
					Server->StopServer();
				else
					Network->Disconnect();
			break;
			case SDL_SCANCODE_GRAVE:
			break;
		}
	}
}

// Mouse handler
void _ClientState::HandleMouseButton(const _MouseEvent &MouseEvent) {
	if(HUD)
		HUD->MouseEvent(MouseEvent);

	if(IsPaused())
		Menu.MouseEvent(MouseEvent);
}

void _ClientState::HandleWindow(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(Graphics.AspectRatio);
}

// Send attack command
void _ClientState::SendAttack() {
	if(!Player)
		return;

	_Buffer Buffer;
	Buffer.Write<char>(Packet::CLIENT_ATTACK);
	Buffer.Write<float>(Player->Physics->Rotation);
	Network->SendPacket(Buffer);
}

// Send use command
void _ClientState::SendUse() {
	if(!Player)
		return;

	_Buffer Buffer;
	Buffer.Write<char>(Packet::CLIENT_USE);
	Network->SendPacket(Buffer);
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
		int InputState = 0;
		if(Actions.State[Action::GAME_UP].Value > 0.0f)
			InputState |= 1 << Action::GAME_UP;
		if(Actions.State[Action::GAME_DOWN].Value > 0.0f)
			InputState |= 1 << Action::GAME_DOWN;
		if(Actions.State[Action::GAME_LEFT].Value > 0.0f)
			InputState |= 1 << Action::GAME_LEFT;
		if(Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			InputState |= 1 << Action::GAME_RIGHT;

		_Controller::_Input Input(TimeSteps, InputState);
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
		Camera->Set2DPosition(glm::vec2(Player->Physics->Position));
		Camera->Update(FrameTime);
	}

	// Update the HUD
	if(HUD)
		HUD->Update(FrameTime);

	// Send network updates to server
	if(Player && Network->IsConnected() && Controller && Controller->History.Size() > 0 /* && Network->NeedsUpdate()*/) {

		// TODO init once
		_Buffer Buffer(200);
		Buffer.Write<char>(Packet::CLIENT_INPUT);

		// Build packet
		Controller->NetworkSerializeHistory(Buffer);

		// Send packet to host
		Network->SendPacket(Buffer, _Network::UNSEQUENCED, 1);

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

	glm::vec3 LightPosition(glm::vec2(Player->Physics->Position), 1.0f);
	glm::vec3 LightAttenuation(0.4f, 0.3f, 0.2f);
	glm::vec4 AmbientLight(0.35f, 0.35f, 0.35f, 1.0f);

	Assets.Programs["pos_uv"]->LightAttenuation = LightAttenuation;
	Assets.Programs["pos_uv"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv"]->AmbientLight = AmbientLight;
	Assets.Programs["pos_uv_norm"]->LightAttenuation = LightAttenuation;
	Assets.Programs["pos_uv_norm"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLight;

	// Setup the viewing matrix
	Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	//TEMP
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw the floor
	Map->RenderFloors();

	// Draw objects
	Map->RenderObjects(BlendFactor, false);

	{
		Graphics.SetDepthTest(false);
		Graphics.SetProgram(Assets.Programs["pos"]);
		glUniformMatrix4fv(Assets.Programs["pos"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 0.3f))));
		Graphics.SetVBO(VBO_NONE);
		Graphics.SetColor(glm::vec4(0, 1, 0, 1));

		_ShotStat ShotStat;
		_Object Object;
		Object.Parent = Player;
		_Shot Shot(Player, &ShotStat);
		Shot.Parent = &Object;
		Shot.Position = glm::vec2(Player->Physics->Position);
		Shot.Direction = glm::normalize(glm::vec2(WorldCursor) - Shot.Position);
		_Impact Impact;
		Map->Grid->CheckBulletCollisions(&Shot, Impact);

		// Draw line
		glm::vec2 StartPosition = glm::vec2(Shot.Position);
		glm::vec2 EndPosition = glm::vec2(Impact.Position);
		float Vertices[] = {
			StartPosition.x, StartPosition.y,
			EndPosition.x, EndPosition.y
		};

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, Vertices);
		glDrawArrays(GL_LINES, 0, 2);
		Graphics.SetDepthTest(true);
	}

	// Setup OpenGL for drawing the HUD
	Graphics.Setup2D();
	//TEMP
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

	HUD->Render();

	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	const _Font *Font = Assets.Fonts["hud_tiny"];
	int X = 60;
	int Y = 20;
	std::ostringstream Buffer;
	Buffer << Network->GetSentSpeed() / 1024.0f << "KB/s";
	Font->DrawText("Send", glm::vec2(X, Y), RIGHT_BASELINE, COLOR_WHITE);
	Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
	Buffer.str("");
	Y += 15;

	Buffer << Network->GetReceiveSpeed() / 1024.0f << "KB/s";
	Font->DrawText("Receive", glm::vec2(X, Y), RIGHT_BASELINE, COLOR_WHITE);
	Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
	Buffer.str("");
	Y += 15;

	Buffer << Network->GetRTT() << "ms";
	Font->DrawText("RTT", glm::vec2(X, Y), RIGHT_BASELINE, COLOR_WHITE);
	Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
	Buffer.str("");
	Y += 15;

	if(Player && Controller) {
		Buffer << Controller->History.Size();
		Font->DrawText("Inputs", glm::vec2(X, Y), RIGHT_BASELINE, COLOR_WHITE);
		Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
		Buffer.str("");
		Y += 15;

		Buffer << TimeSteps;
		Font->DrawText("TimeStep", glm::vec2(X, Y), RIGHT_BASELINE, COLOR_WHITE);
		Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
		Buffer.str("");
		Y += 15;
	}

	if(Map) {
		Buffer << Map->GetObjectCount();
		Font->DrawText("Objects", glm::vec2(X, Y), RIGHT_BASELINE, COLOR_WHITE);
		Font->DrawText(Buffer.str(), glm::vec2(X+10, Y));
		Buffer.str("");
		Y += 15;
	}
	Graphics.SetDepthMask(true);
}

bool _ClientState::IsPaused() {
	return false;
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
		case Packet::UPDATE_HEALTH:
			HandleUpdateHealth(Buffer);
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
		Level = "test.map";

	_Buffer Buffer;
	Buffer.Write<char>(Packet::CLIENT_JOIN);
	Buffer.WriteString(Level.c_str());
	Network->SendPacket(Buffer);

	// Initialize hud
	HUD = new _HUD();

	// Set up graphics
	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(Graphics.AspectRatio);
}

// Load the map
void _ClientState::HandleMapInfo(_Buffer &Buffer) {

	// Read packet
	uint8_t MapID = Buffer.Read<uint8_t>();
	std::string NewMap = Buffer.ReadString();

	// Create new map
	delete Map;
	Map = new _Map(NewMap, Stats, false, MapID);
	Map->SetCamera(Camera);
	Player = nullptr;
	Controller = nullptr;
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
		Map->Grid->AddObject(Object);

		// Keep track of player id
		if(ClientID == NetworkID)
			Player = Object;
	}

	if(Player) {
		Controller = (_Controller *)Player->Components["controller"];
		Player->Log = Log;
		Player->Physics->RenderDelay = false;
		Player->Physics->UpdateAutomatically = false;
		Camera->ForcePosition(glm::vec3(Player->Physics->Position.x, Player->Physics->Position.y, CAMERA_DISTANCE));
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
	if(Controller)
		Controller->ReplayInput();

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
	if(Object->Shape)
		Map->Grid->AddObject(Object);

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

// Handle object health update
void _ClientState::HandleUpdateHealth(_Buffer &Buffer) {
	uint16_t ID = Buffer.Read<uint16_t>();
	uint16_t NewHealth = Buffer.Read<int>();

	_Object *Object = Map->GetObjectByID(ID);
	if(Object && Object->HasComponent("health")) {
		_Health *Health = (_Health *)(Object->Components["health"]);
		Health->Health = NewHealth;
		std::cout << "Health update object_id=" << ID << ", health=" << Health->Health << std::endl;
	}
}
