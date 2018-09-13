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
#include <objects/ai.h>
#include <objects/physics.h>
#include <objects/object.h>
#include <stats.h>
#include <ae/buffer.h>
#include <map.h>
#include <constants.h>
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
void _Ai::Update(double FrameTime) {
	if(!Parent->Server)
		return;

	_Physics *Physics = Parent->Physics;

	//TODO fix
	Parent->SendUpdate = true;

	// Follow target
	if(Target) {
		Physics->Velocity = Target->Physics->Position - Physics->Position;

		float TargetRadians = (Target->Physics->Rotation - 90) / (180.0f / MATH_PI);
		glm::vec2 TargetDirection = glm::vec2(std::cos(TargetRadians), std::sin(TargetRadians));

		if(glm::dot(glm::vec2(Physics->Velocity), TargetDirection) > 0) {
			if(!(Physics->Velocity.x == 0.0f && Physics->Velocity.y == 0.0f)) {
				Physics->Velocity = glm::normalize(Physics->Velocity) * 0.01f;
				Parent->SendUpdate = true;
			}
			//if(TimeSteps & 64) {
				//Physics->Velocity = glm::vec3(0, 0, 0);
			//}
		}
		else
			Physics->Velocity = glm::vec3(0, 0, 0);
	}
	else {
		Physics->Velocity = glm::vec3(0, 0, 0);
		TargetTimer += FrameTime;
		if(TargetTimer >= 1.0)
			FindTarget();

		//std::cout << "TargetTimer=" << TargetTimer << std::endl;
	}
}

// Serialize
void _Ai::NetworkSerialize(ae::_Buffer &Buffer) {
}

// Unserialize
void _Ai::NetworkUnserialize(ae::_Buffer &Buffer) {
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
