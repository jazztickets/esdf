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
#include <objects/controller.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/animation.h>
#include <ae/network.h>
#include <constants.h>
#include <stats.h>
#include <ae/actions.h>
#include <ae/buffer.h>
#include <actiontype.h>
#include <glm/gtx/norm.hpp>

// Constructor
_Controller::_Controller(_Object *Parent, const _ControllerStat *Stats) :
	_Component(Parent),
	Stats(Stats),
	LastInputTime(0) {

	History.Init(200);
}

// Destructor
_Controller::~_Controller() {
}

// Serialize update
void _Controller::NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
	Buffer.Write<uint16_t>(LastInputTime);
}

// Unserialize update
void _Controller::NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
	LastInputTime = Buffer.Read<uint16_t>();
}

// Handles player input
void _Controller::HandleInput(const _Input &Input, bool ReplayingInput) {

	// Get move direction
	glm::vec2 Direction(0.0f, 0.0f);
	if(Input.ActionState & (1 << Action::GAME_LEFT))
		Direction.x -= 1.0f;
	if(Input.ActionState & (1 << Action::GAME_RIGHT))
		Direction.x += 1.0f;
	if(Input.ActionState & (1 << Action::GAME_UP))
		Direction.y -= 1.0f;
	if(Input.ActionState & (1 << Action::GAME_DOWN))
		Direction.y += 1.0f;

	if(!(Direction.x == 0 && Direction.y == 0)) {
		//if(Parent->Animation)
		//	Parent->Animation->Play(0);
		Direction = glm::normalize(Direction);
	}
	else {
		//if(Parent->Animation)
		//	Parent->Animation->Stop();
	}

	Parent->Physics->Velocity = glm::vec3(Direction * Stats->Speed, 0.0f);

	//if(Input.ActionState & (1 << Action::GAME_FIRE))
	//	StartAttack();
	//if(Player->GetFireRate() == FIRERATE_AUTO && Actions.GetState(Action::GAME_FIRE))

	//if(Input & (1 << _Actions::USE))
	//	StartUse();

	//SetCrouching(Input & (1 << _Actions::AIM));

	//if(((LastInput ^ Input) & (1 << _Actions::INVENTORY)) && (Input & (1 << _Actions::INVENTORY)))
	//	HUD->SetInventoryOpen(!HUD->GetInventoryOpen());
		//Player->SetCrouching(false);

	//LastInput = Input.ActionState;
}

// Handle mouse movement
void _Controller::HandleCursor(const glm::vec2 &Cursor) {
	Parent->Physics->FacePosition(Cursor);
}

// Rewind the physics state and replay input
void _Controller::ReplayInput() {

	// Find start time
	int PredictionIndex = -1;
	for(int i = 0; i < History.Size(); i++) {
		const _Input &Input = History.Back(i);
		if(Input.Time == LastInputTime) {
			PredictionIndex = i-1;
			break;
		}
	}

	// Apply client correction
	if(PredictionIndex >= 0) {
		Parent->Physics->Position = Parent->Physics->NetworkPosition;
		while(PredictionIndex >= 0) {
			const _Input &Input = History.Back(PredictionIndex);
			HandleInput(Input, true);
			Parent->Physics->Update(GAME_TIMESTEP);
			PredictionIndex--;
		}
	}

	// Remove old inputs from send queue
	while(!History.IsEmpty()) {
		const _Input &Input = History.Front();
		if(!_Network::MoreRecentAck(Input.Time, LastInputTime, uint16_t(-1)))
			break;

		History.Pop();
	}
}

// Serialize input history
void _Controller::NetworkSerializeHistory(_Buffer &Buffer) {

	// Get count of inputs we're about to send
	int InputCount = History.Size();

	Buffer.Write<float>(Parent->Physics->Rotation);
	Buffer.Write<uint16_t>(History.Front(0).Time);

	// Start key header
	uint8_t *LastByte = Buffer.Write<uint8_t>(0);

	// Delta compress keys
	int KeyCount = 0;
	uint8_t LastKey = History.Front(0).ActionState;
	for(int i = 1; i < InputCount; i++) {
		uint8_t Key = History.Front(i).ActionState;
		if(Key != LastKey || KeyCount == 15) {

			// Write key state out - 4 bits for key count, 4 bits for key state
			*LastByte = (KeyCount << 4) | LastKey;

			// Write new key header
			LastByte = Buffer.Write<uint8_t>(0);
			LastKey = Key;
			KeyCount = 0;
		}
		else {
			KeyCount++;
		}
	}

	// Write final key state
	*LastByte = (KeyCount << 4) | LastKey;
}
