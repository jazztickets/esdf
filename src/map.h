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
#include <ae/network.h>
#include <ae/type.h>
#include <ae/baseobject.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/fwd.hpp>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>

// Forward Declarations
class _Object;
class _ObjectManager;
class _Scripting;
class _Server;
class _Stats;
class _Grid;

namespace ae {
	template<class T> class _Manager;
	class _ServerNetwork;
	class _Atlas;
	class _Camera;
	class _Texture;
	class _Peer;
	class _Buffer;
	struct _Layer;
}

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
	const ae::_Layer *Layer;
};

// Classes
class _Map : public ae::_BaseObject {

	public:

		_Map();
		~_Map();

		bool Save(const std::string &Path);
		void Load(const std::string &Path, const _Stats *Stats, ae::_Manager<_Object> *ObjectManager, ae::_ServerNetwork *ServerNetwork=nullptr);

		void Update(double FrameTime);

		void SetCamera(ae::_Camera *Camera) { this->Camera = Camera; }
		void RenderFloors();
		void RenderObjects(double BlendFactor, bool EditorOnly);
		void RenderGrid(int Spacing);
		void HighlightBlocks();

		glm::vec2 GetStartingPositionByCheckpoint(int Level);
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const;

		// Objects
		void AddObject(_Object *Object);
		void RemoveObject(_Object *Object);
		void BroadcastPacket(ae::_Buffer &Buffer, ae::_Network::SendType Type=ae::_Network::RELIABLE);
		void SendObjectList(_Object *Player, uint16_t TimeSteps);
		void SendObjectUpdates(uint16_t TimeSteps);
		void GetSelectedObjects(const glm::vec4 &AABB, std::list<_Object *> &SelectedObjects);
		void QueryObjects(const glm::vec2 &Position, float Radius, std::list<_Object *> &QueriedObjects);
		size_t GetObjectCount() { return Objects.size(); }

		// Network
		const std::list<const ae::_Peer *> &GetPeers() const { return Peers; }
		void AddPeer(const ae::_Peer *Peer) { Peers.push_back(Peer); }
		void RemovePeer(const ae::_Peer *Peer);

		static std::string FixFilename(const std::string &Filename);

		// Attributes
		std::string Filename;
		const ae::_Atlas *TileAtlas;

		// Rendering
		std::vector<_RenderList> RenderList;

		// Collision
		_Grid *Grid;

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
		ae::_Camera *Camera;

		// Network
		ae::_ServerNetwork *ServerNetwork;
		std::list<const ae::_Peer *> Peers;
		uint16_t ObjectUpdateCount;
};
