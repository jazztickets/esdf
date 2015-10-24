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
#pragma once

// Libraries
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
class _Shape;
class _Buffer;
class _Peer;
class _Map;
class _LogFile;

// Object class
class _Object {

	public:

		_Object();
		~_Object();

		void Update(double FrameTime, uint16_t TimeSteps);

		// Network
		void NetworkSerialize(_Buffer &Buffer);
		void NetworkUnserialize(_Buffer &Buffer);
		void NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps);
		void NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps);

		// Collision
		bool CheckCircle(const glm::vec2 &Position, float Radius, glm::vec2 &Push, bool &AxisAlignedPush);
		bool CheckAABB(const glm::vec4 &AABB);

		inline bool HasComponent(const std::string &Name) { return Components.find(Name) != Components.end(); }

		// Components
		_Physics *Physics;
		_Animation *Animation;
		_Render *Render;
		_Shape *Shape;

		std::unordered_map<std::string, _Component *> Components;

		// Pointers
		_Object *Parent;
		_Peer *Peer;
		_Map *Map;
		_LogFile *Log;

		// Attributes
		float Lifetime;
		bool Deleted : 1;
		bool SendUpdate : 1;
		bool Server : 1;
		bool Event : 1;
		uint16_t ID;
		std::string Identifier;
		std::string Name;

};
