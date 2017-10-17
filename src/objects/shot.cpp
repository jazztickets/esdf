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
#include <objects/shot.h>
#include <objects/object.h>
#include <objects/health.h>
#include <ae/network.h>
#include <map.h>
#include <grid.h>
#include <ae/buffer.h>
#include <packet.h>
#include <constants.h>

_Shot::_Shot(_Object *Parent, const _ShotStat *Stats)
:	_Component(Parent) {
}

// Serialize
void _Shot::NetworkSerialize(_Buffer &Buffer) {
	Buffer.Write<glm::vec2>(Position);
	Buffer.Write<float>(Rotation);
}

// Unserialize
void _Shot::NetworkUnserialize(_Buffer &Buffer) {
	Position = Buffer.Read<glm::vec2>();
	Rotation = Buffer.Read<float>();

	CalcDirectionFromRotation();
}

// Update
void _Shot::Update(double FrameTime) {
	if(!Parent->Server)
		return;

	// Check collisions
	_Impact Impact;
	Parent->Map->Grid->CheckBulletCollisions(this, Impact);
	EndPosition = Impact.Position;

	// Notify clients
	if(Impact.Object) {

		// Check for health
		if(Impact.Object->HasComponent("health")) {
			_Health *Health = (_Health *)(Impact.Object->Components["health"]);

			// Update health
			Health->Health -= 10;
			if(Health->Health <= 0) {
				Health->Health = 0;
				if(Impact.Object->Identifier != "player")
					Impact.Object->Deleted = true;
			}

			_Buffer Buffer;
			Buffer.Write<char>(Packet::UPDATE_HEALTH);
			Buffer.Write<uint16_t>(Impact.Object->NetworkID);
			Buffer.Write<int>(Health->Health);

			// Broadcast to all other peers
			Parent->Map->BroadcastPacket(Buffer, _Network::RELIABLE);
		}
	}
}

// Get a direction vector for degrees
void _Shot::CalcDirectionFromRotation() {
	float Radians = (Rotation - 90) / (180.0f / MATH_PI);
	Direction = glm::vec2(std::cos(Radians), std::sin(Radians));
}
