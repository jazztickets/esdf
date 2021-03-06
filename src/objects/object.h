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
#include <ae/baseobject.h>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <unordered_map>
#include <vector>
#include <string>

// Forward Declarations
class _Component;
class _Physics;
class _Animation;
class _Render;
class _CollisionShape;
class _Map;

namespace ae {
	class _Peer;
	class _Buffer;
	class _LogFile;
}

// Object class
class _Object : public ae::_BaseObject {

	public:

		_Object();
		~_Object();

		// Updates
		void Update(double FrameTime);

		// Network
		void NetworkSerialize(ae::_Buffer &Buffer);
		void NetworkUnserialize(ae::_Buffer &Buffer);
		void NetworkSerializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps);
		void NetworkUnserializeUpdate(ae::_Buffer &Buffer, uint16_t TimeSteps);

		// Collision
		bool CheckCircle(const glm::vec2 &Position, float Radius, glm::vec2 &Push, bool &AxisAlignedPush);
		bool CheckAABB(const glm::vec4 &AABB);

		inline bool HasComponent(const std::string &Name) { return Components.find(Name) != Components.end(); }

		// Components
		_Physics *Physics;
		_Animation *Animation;
		_Render *Render;
		_CollisionShape *Shape;

		std::unordered_map<std::string, _Component *> Components;

		// Pointers
		_Object *Parent;
		ae::_Peer *Peer;
		_Map *Map;
		ae::_LogFile *Log;

		// Attributes
		uint16_t TimeSteps;
		float Lifetime;
		bool SendUpdate;
		bool Server;
		bool Event;
		std::string Identifier;
		std::string Name;

};
