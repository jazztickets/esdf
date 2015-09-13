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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/detail/func_common.hpp>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>

// Forward Declarations
class _Object;
class _Camera;
class _Texture;
class _Atlas;
class _Particles;
class _ObjectManager;
class _Peer;
class _Buffer;
class _Scripting;
class _Server;
class _Stats;
class _Grid;
class _ServerNetwork;
struct _Layer;

// Holds information about a hit entity
struct _Impact {

	enum ImpactType {
		NONE,
		OBJECT,
		WALL,
	};

	_Object *Object;
	glm::vec2 Position;
	float Distance;
	int Type;
};

struct _RenderList {
	std::list<_Object *> Objects;
	const _Layer *Layer;
};

// Classes
class _Map {

	public:

		_Map();
		_Map(const std::string &Path, const _Stats *Stats, uint8_t ID=0, _ServerNetwork *ServerNetwork=nullptr);
		~_Map();

		bool Save(const std::string &String);

		void SetCamera(_Camera *Camera) { this->Camera = Camera; }
		void SetParticles(_Particles *Particles) { this->Particles = Particles; }

		void Update(double FrameTime, uint16_t TimeSteps);

		void RenderFloors();
		void RenderObjects(double BlendFactor, bool EditorOnly);
		void RenderGrid(int Spacing, float *Vertices);
		void HighlightBlocks();

		void GetSelectedObjects(const glm::vec4 &AABB, std::list<_Object *> *SelectedObjects);

		glm::vec2 GetStartingPositionByCheckpoint(int Level);
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const;

		void DeleteObjects();
		void RemoveObject(_Object *Object);

		void BroadcastPacket(_Buffer &Buffer);
		void SendObjectList(_Object *Player, uint16_t TimeSteps);
		void BuildObjectUpdate(_Buffer &Buffer, uint16_t TimeSteps);
		void BuildObjectList(_Buffer &Buffer);
		void UpdateObjectsFromBuffer(_Buffer &Buffer, uint16_t TimeSteps);
		_Object *GetObjectByID(uint16_t ObjectID);

		const std::list<const _Peer *> &GetPeers() const { return Peers; }
		void AddPeer(const _Peer *Peer) { Peers.push_back(Peer); }
		void RemovePeer(const _Peer *Peer);
		void AddObject(_Object *Object) { Objects.push_back(Object); }

		// Attributes
		std::string Filename;
		uint8_t ID;
		const _Atlas *TileAtlas;

		// Rendering
		std::vector<_RenderList> RenderList;

		// Collision
		_Grid *Grid;

		// Network
		uint16_t NextObjectID;

		// Stats
		const _Stats *Stats;

		// Scripting
		_Scripting *Scripting;

	private:

		// Objects
		std::list<_Object *> Objects;

		// Rendering
		uint32_t TileVertexBufferID;
		uint32_t TileElementBufferID;
		glm::vec4 *TileVertices;
		glm::u32vec3 *TileFaces;

		// Graphics
		_Camera *Camera;
		_Particles *Particles;

		// Network
		_ServerNetwork *ServerNetwork;
		std::list<const _Peer *> Peers;
		uint16_t ObjectUpdateCount;
};
