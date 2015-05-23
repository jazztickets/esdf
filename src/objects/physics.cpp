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
#include <objects/physics.h>
#include <objects/object.h>
#include <objects/animation.h>
#include <objects/shape.h>
#include <network/network.h>
#include <constants.h>
#include <map.h>
#include <grid.h>
#include <stats.h>
#include <buffer.h>
#include <cmath>
#include <map>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Constructor
_Physics::_Physics(_Object *Parent) :
	Parent(Parent),
	Position(0),
	LastPosition(0),
	NetworkPosition(0),
	Velocity(0),
	Rotation(0.0f),
	InterpolatedRotation(0.0f),
	Interpolate(true),
	ClientSidePrediction(false) {

	History.Init(20);
}

// Destructor
_Physics::~_Physics() {
}

// Serialize
void _Physics::NetworkSerialize(_Buffer &Buffer) {
	Buffer.Write<glm::vec2>(glm::vec2(Position.x, Position.y));
	Buffer.Write<float>(Rotation);
}

// Unserialize
void _Physics::NetworkUnserialize(_Buffer &Buffer) {
	NetworkPosition = Position = glm::vec3(Buffer.Read<glm::vec2>(), 0.0f);
	InterpolatedRotation = Rotation = Buffer.Read<float>();
}

// Serialize update
void _Physics::NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
	Buffer.Write<glm::vec2>(glm::vec2(Position.x, Position.y));
	Buffer.Write<float>(Rotation);
}

// Unserialize update
void _Physics::NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
	NetworkPosition = glm::vec3(Buffer.Read<glm::vec2>(), 0.0f);
	Rotation = Buffer.Read<float>();

	History.PushBack(_History(NetworkPosition, TimeSteps));
}

// Set rotation to face a position
void _Physics::FacePosition(const glm::vec2 &Cursor) {

	Rotation = std::atan2(Cursor.y - Position.y, Cursor.x - Position.x) * (180.0f / MATH_PI) + 90.0f;
	if(Rotation < 0.0f)
		Rotation += 360.0f;
}

// Force position
void _Physics::ForcePosition(const glm::vec2 &Position) {
	LastPosition = this->Position = glm::vec3(Position, 0.0f);
}

// Update
void _Physics::Update(double FrameTime, uint16_t TimeSteps) {
	LastPosition = Position;

	// Apply render delay to other players
	if(Interpolate && History.Size() >= 3) {

		// Get rendertime
		uint16_t RenderTime = TimeSteps - 6;

		// Find the timestep to the right of the rendertime
		int End = 0;
		for(int i = 0; i < History.Size(); i++) {
			End = i-1;
			if(_Network::MoreRecentAck(History.Back(i).Time, RenderTime, uint16_t(-1))) {
				break;
			}
		}

		// Check for no positions found within buffer time, then extrapolate
		int Start;
		int InterpolationIndex;
		if(End == -1) {
			End = 0;
			Start = 1;
			InterpolationIndex = End;
		}
		else {
			Start = End + 1;
			InterpolationIndex = Start;
		}

		// Get interpolation amount
		float Percentage = float(RenderTime - History.Back(InterpolationIndex).Time) / (History.Back(End).Time - History.Back(Start).Time);
		glm::vec3 DeltaPosition = History.Back(End).Position - History.Back(Start).Position;
		LastPosition = Position;
		Parent->Map->Grid->RemoveObject(Parent);
		Position = History.Back(InterpolationIndex).Position + DeltaPosition * Percentage;

		if(Position != LastPosition) {
			Parent->Animation->Play(0);
		}
		else
			Parent->Animation->Stop();
		Parent->Map->Grid->AddObject(Parent);

		// Update rotation
		float DeltaRotation = Rotation - InterpolatedRotation;
		if(DeltaRotation < -180.0f)
			DeltaRotation += 360.0f;
		else if(DeltaRotation > 180.0f)
			DeltaRotation -= 360.0f;

		InterpolatedRotation += DeltaRotation * 0.4f;
		if(InterpolatedRotation < 0.0f)
			InterpolatedRotation += 360.0f;
		else if(InterpolatedRotation >= 360.0f)
			InterpolatedRotation -= 360.0f;
	}

	if(!Interpolate) {

		// Get a list of entities that the object is colliding with
		std::unordered_map<_Object *, bool> HitEntities;
		Parent->Map->Grid->CheckEntityCollisionsInGrid(glm::vec2(Parent->Physics->Position), Parent->Shape->Stat.HalfWidth[0], Parent, HitEntities);

		// Limit movement
		for(auto Iterator : HitEntities) {
			glm::vec3 HitObjectDirection = Iterator.first->Physics->Position - Parent->Physics->Position;

			// Determine if we need to clip the direction
			if(glm::dot(HitObjectDirection, Velocity) > 0) {

				// Get a vector that divides the two objects
				glm::vec3 DividingLine = glm::normalize(glm::vec3(-HitObjectDirection.y, HitObjectDirection.x, 0.0f));

				// Project the velocity vector onto the dividing line
				Velocity = DividingLine * glm::dot(Velocity, DividingLine);
			}
		}

		// Check collisions with walls and map boundaries
		glm::vec3 NewPosition = Parent->Physics->Position + Velocity;

		// Get list of blocks that the object is potentially touching
		std::map<_Block *, bool> PotentialBlocks;

		// Get AABB of object
		_TileBounds TileBounds;
		Parent->Map->Grid->GetTileBounds(glm::vec2(NewPosition), Parent->Shape->Stat.HalfWidth[0], TileBounds);
		for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
			for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
				for(auto Iterator = Parent->Map->Grid->Tiles[i][j].Blocks.begin(); Iterator != Parent->Map->Grid->Tiles[i][j].Blocks.end(); ++Iterator) {
					PotentialBlocks[*Iterator] = true;
				}
			}
		}

		Parent->Map->CheckCollisions(NewPosition, Parent->Shape->Stat.HalfWidth[0], PotentialBlocks);

		// Determine if the object has moved
		if(Parent->Physics->Position != NewPosition) {

			// Update grid and position
			Parent->Map->Grid->RemoveObject(Parent);
			Parent->Physics->Position = NewPosition;
			Parent->Map->Grid->AddObject(Parent);

			//PositionChanged = true;
			if(Parent->Animation)
				Parent->Animation->Play(0);
		}
		else {
			if(Parent->Animation)
				Parent->Animation->Stop();
			//PositionChanged = false;
		}
	}
}
