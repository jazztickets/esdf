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
#include <objects/ai.h>
#include <objects/physics.h>
#include <objects/object.h>
#include <stats.h>
#include <buffer.h>
#include <map.h>
#include <glm/gtx/norm.hpp>
#include <iostream>

// Constructor
_Ai::_Ai(_Object *Parent, const _AiStat *Stat) :
	_Component(Parent),
	Target(nullptr),
	TargetTimer(0.0) {

}

// Destructor
_Ai::~_Ai() {
}

// Update
void _Ai::Update(double FrameTime, uint16_t TimeSteps) {
	if(!Parent->Server)
		return;

	_Physics *Physics = Parent->Physics;

	// Follow target
	if(Target) {
		Physics->Velocity = Target->Physics->Position - Physics->Position;
		if(!(Physics->Velocity.x == 0.0f && Physics->Velocity.y == 0.0f)) {
			Physics->Velocity = glm::normalize(Physics->Velocity) * 0.01f;
			Parent->SendUpdate = true;
		}
	}
	else {
		Physics->Velocity = glm::vec3(0, 0, 0);
		Parent->SendUpdate = false;
		TargetTimer += FrameTime;
		if(TargetTimer >= 1.0)
			FindTarget();

		//std::cout << "TargetTimer=" << TargetTimer << std::endl;
	}
}

// Serialize
void _Ai::NetworkSerialize(_Buffer &Buffer) {
}

// Unserialize
void _Ai::NetworkUnserialize(_Buffer &Buffer) {
}

// Find a new target
void _Ai::FindTarget() {
	//std::cout << "Finding target" << std::endl;
	TargetTimer = 0.0;
	if(Parent->Map) {
		_Physics *Physics = Parent->Physics;
		glm::vec4 AABB(Physics->Position.x - 5, Physics->Position.y - 5, Physics->Position.x + 5, Physics->Position.y + 5);
		std::list<_Object *> Objects;
		Parent->Map->GetSelectedObjects(AABB, Objects);
		for(auto &Object : Objects) {
			if(Object->Identifier == "player") {
				//std::cout << "Found player" << std::endl;
				Target = Object;
				break;
			}
		}
	}
}
