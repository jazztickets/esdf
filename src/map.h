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
#include <unordered_map>

// Forward Declarations
class _Object;
class _Shot;
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

// Holds data for a block of tiles
struct _Block {
	glm::vec4 GetAABB() const { return glm::vec4(Position.x - HalfWidth.x,
												 Position.y - HalfWidth.y,
												 Position.x + HalfWidth.x,
												 Position.y + HalfWidth.y); }

	glm::vec3 GetStart() const { return glm::vec3(Position - HalfWidth); }
	glm::vec3 GetEnd() const { return glm::vec3(Position + HalfWidth); }

	glm::vec3 Position;
	glm::vec3 HalfWidth;
	const _Texture *Texture;
};

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

// Classes
class _Map {

	public:

		_Map();
		_Map(const std::string &Path, const _Stats *Stats, uint8_t ID=0, _ServerNetwork *ServerNetwork=nullptr);
		~_Map();

		bool Save(const std::string &String);

		void SetCamera(_Camera *Camera) { this->Camera = Camera; }
		void SetParticles(_Particles *Particles) { this->Particles = Particles; }
		void SetAmbientLight(const glm::vec4 &Color) { OldAmbientLight = AmbientLight; AmbientLight = Color; }
		void SetAmbientLightChangePeriod(double Value) { AmbientLightPeriod = Value; AmbientLightTimer = AmbientLightBlendFactor = 0.0; }
		void SetAmbientLightRadius(float Value) { AmbientLightRadius = Value; }

		void Update(double FrameTime, uint16_t TimeSteps);
		bool CheckCollisions(glm::vec2 &Position, float Radius);

		void RenderFloors();
		void RenderWalls(_Block *ExceptionBlock);
		void RenderObjects(double BlendFactor);
		void RenderGrid(int Spacing, float *Vertices);
		void HighlightBlocks();

		void AddBlock(_Block *Block);
		void GetSelectedObjects(const glm::vec4 &AABB, std::list<_Object *> *SelectedObjects);
		_Block *GetSelectedBlock(const glm::vec2 &Position);
		void RemoveBlock(const _Block *Block);

		glm::vec2 GetStartingPositionByCheckpoint(int Level);
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const;

		void DeleteObjects();
		void RemoveObject(_Object *Object);

		void AddShot(const _Shot *Shot);

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

		// Collision
		_Grid *Grid;

		// Network
		uint16_t NextObjectID;

		// Stats
		const _Stats *Stats;

	private:

		bool ResolveCircleAABBCollision(const glm::vec2 &Position, float Radius, const glm::vec4 &AABB, bool Resolve, glm::vec2 &Push, bool &DiagonalPush);
		void UpdateShots();

		// Scripting
		_Scripting *Scripting;

		// Blocks
		std::list<_Block *> Blocks;

		// Objects
		std::list<_Object *> Objects;
		std::list<const _Shot *> Shots;

		// Rendering
		std::list<_Object *> RenderList[4];
		uint32_t TileVertexBufferID;
		uint32_t TileElementBufferID;
		glm::vec4 *TileVertices;
		glm::u32vec3 *TileFaces;

		// Graphics
		_Camera *Camera;
		_Particles *Particles;

		// Lights
		glm::vec4 AmbientLight;
		glm::vec4 OldAmbientLight;
		float AmbientLightRadius;
		double AmbientLightBlendFactor;
		double AmbientLightPeriod;
		double AmbientLightTimer;

		// Network
		_ServerNetwork *ServerNetwork;
		std::list<const _Peer *> Peers;
		uint16_t ObjectUpdateCount;
};
