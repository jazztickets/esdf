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
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/controller.h>
#include <objects/animation.h>
#include <objects/render.h>
#include <objects/shape.h>
#include <objects/shot.h>
#include <map.h>
#include <constants.h>
#include <ae/buffer.h>
#include <glm/gtx/norm.hpp>
#include <iostream>

// Constructor
_Object::_Object() :
	Physics(nullptr),
	Animation(nullptr),
	Render(nullptr),
	Shape(nullptr),
	Parent(nullptr),
	Peer(nullptr),
	Map(nullptr),
	Log(nullptr),
	TimeSteps(0),
	Lifetime(-1),
	SendUpdate(false),
	Server(false),
	Event(false),
	Identifier(""),
	Name("") {

}

// Destructor
_Object::~_Object() {
	if(Map) {
		Map->RemoveObject(this);
		Map = nullptr;
	}

	for(auto &Component : Components)
		delete Component.second;
}

// Update
void _Object::Update(double FrameTime) {

	// Update components
	for(auto &Component : Components) {
		if(Component.second->UpdateAutomatically)
			Component.second->Update(FrameTime);
	}

	// Update lifetime
	if(Lifetime > 0.0f) {
		Lifetime -= FrameTime;
		if(Lifetime < 0.0f)
			Lifetime = 0.0f;
	}

	// Delete object
	if(Lifetime == 0.0f)
		Deleted = true;
}

// Serialize components
void _Object::NetworkSerialize(ae::_Buffer &Buffer) {
	Buffer.WriteString(Identifier.c_str());
	Buffer.Write<ae::NetworkIDType>(NetworkID);

	for(auto &Component : Components)
		Component.second->NetworkSerialize(Buffer);
}

// Unserialize components
void _Object::NetworkUnserialize(ae::_Buffer &Buffer) {

	for(auto &Component : Components)
		Component.second->NetworkUnserialize(Buffer);
}

// Serialize update
void _Object::NetworkSerializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps) {
	Buffer.Write<ae::NetworkIDType>(NetworkID);

	if(HasComponent("controller")) {
		_Controller *Controller = (_Controller *)Components["controller"];
		Controller->NetworkSerializeUpdate(Buffer, TimeSteps);
	}

	if(Physics)
		Physics->NetworkSerializeUpdate(Buffer, TimeSteps);
}

// Unserialize update
void _Object::NetworkUnserializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps) {

	if(HasComponent("controller")) {
		_Controller *Controller = (_Controller *)Components["controller"];
		Controller->NetworkUnserializeUpdate(Buffer, TimeSteps);
	}

	if(Physics)
		Physics->NetworkUnserializeUpdate(Buffer, TimeSteps);
}

// Check collision with a min max AABB
bool _Object::CheckAABB(const glm::vec4 &AABB) {
	if(!Shape)
		return true;

	// Shape is AABB
	if(Shape->IsAABB()) {

		if(Physics->Position.x - Shape->HalfWidth[0] >= AABB[2])
			return false;

		if(Physics->Position.y - Shape->HalfWidth[1] >= AABB[3])
			return false;

		if(Physics->Position.x + Shape->HalfWidth[0] <= AABB[0])
			return false;

		if(Physics->Position.y + Shape->HalfWidth[1] <= AABB[1])
			return false;

	}
	else {

		// Get closest point on AABB
		glm::vec3 Point = Physics->Position;
		if(Point.x < AABB[0])
			Point.x = AABB[0];
		if(Point.y < AABB[1])
			Point.y = AABB[1];
		if(Point.x > AABB[2])
			Point.x = AABB[2];
		if(Point.y > AABB[3])
			Point.y = AABB[3];

		float DistanceSquared = glm::distance2(Point, Physics->Position);
		return DistanceSquared < Shape->HalfWidth[0] * Shape->HalfWidth[0];
	}

	return true;
}

// Check collision with a circle
bool _Object::CheckCircle(const glm::vec2 &Position, float Radius, glm::vec2 &Push, bool &AxisAlignedPush) {

	// Get vector to circle center
	glm::vec2 Point = Position - glm::vec2(Physics->Position);

	// Shape is AABB
	if(Shape->IsAABB()) {

		glm::vec2 ClosestPoint = Point;
		int ClampCount = 0;
		if(ClosestPoint.x < -Shape->HalfWidth[0]) {
			ClosestPoint.x = -Shape->HalfWidth[0];
			ClampCount++;
		}
		if(ClosestPoint.y < -Shape->HalfWidth[1]) {
			ClosestPoint.y = -Shape->HalfWidth[1];
			ClampCount++;
		}
		if(ClosestPoint.x > Shape->HalfWidth[0]) {
			ClosestPoint.x = Shape->HalfWidth[0];
			ClampCount++;
		}
		if(ClosestPoint.y > Shape->HalfWidth[1]) {
			ClosestPoint.y = Shape->HalfWidth[1];
			ClampCount++;
		}

		bool Hit = glm::distance2(Point, ClosestPoint) < Radius * Radius;
		if(Hit) {

			// Get push direction
			Push = Point - ClosestPoint;

			// Check for zero vector
			if(Push.x == 0.0f && Push.y == 0.0f) {
				Push.x = 1.0f;
				return true;
			}

			// Get push amount
			float Amount = Radius - glm::length(Push);

			// Scale push vector
			Push = glm::normalize(Push);
			Push *= Amount;

			if(ClampCount == 1)
				AxisAlignedPush = true;

			return true;
		}
	}
	else {

		float SquareDistance = Point.x * Point.x + Point.y * Point.y;
		float RadiiSum = Radius + Shape->HalfWidth[0];

		bool Hit = SquareDistance < RadiiSum * RadiiSum;
		if(Hit) {

			// Check for zero vector
			if(Point.x == 0.0f && Point.y == 0.0f) {
				Push.x = 1.0f;
				return true;
			}

			// Get push direction
			Push = glm::normalize(Point);
			Push *= RadiiSum - sqrtf(SquareDistance);

			return true;
		}
	}

	return false;
}
