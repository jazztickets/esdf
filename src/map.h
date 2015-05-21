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
class _ServerNetwork;
struct _Spawn;

// Types of objects in the collision grid
enum CollisionGridType {
	GRID_PLAYER,
	GRID_MONSTER,
	GRID_ITEM,
	GRID_COUNT
};

// Holds data for a tile bound
struct _TileBounds {
	glm::ivec2 Start;
	glm::ivec2 End;
};

// Holds data for a block of tiles
struct _Block {
	glm::vec3 Start;
	glm::vec3 End;
	const _Texture *Texture;
	int Collision;
};

// Holds data for a single tile
struct _Tile {
	enum CollisionFlagType {
		ENTITY = 1,
		BULLET = 2,
	};

	_Tile() : TextureIndex(0) { }

	std::list<_Object *> Objects[GRID_COUNT];
	std::list<_Block *> Blocks;
	int TextureIndex;
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

// Object spawn struct
struct _Spawn {
	_Spawn() { }
	_Spawn(const std::string &Identifier, const glm::vec2 &Position) : Identifier(Identifier), Position(Position) { }

	std::string Identifier;
	glm::vec2 Position;
};

// Classes
class _Map {

	public:

		_Map();
		_Map(const std::string &Path, const _Stats *Stats, uint8_t ID=0, _ServerNetwork *ServerNetwork=nullptr);
		~_Map();

		void SetCamera(_Camera *Camera) { this->Camera = Camera; }
		void SetParticles(_Particles *Particles) { this->Particles = Particles; }

		void Update(double FrameTime, uint16_t TimeSteps);

		bool Save(const std::string &String);

		bool CheckCollisions(glm::vec2 &Position, float Radius);
		void CheckEntityCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, std::unordered_map<_Object *, bool> &Entities) const;
		_Object *CheckCollisionsInGrid(const glm::vec2 &Position, float Radius, int GridType, const _Object *SkipObject) const;
		void CheckBulletCollisions(const _Shot *Shot, _Impact &Impact, bool CheckObjects) const;
		float RayObjectIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction, const _Object *Object) const;
		bool IsVisible(const glm::vec2 &Start, const glm::vec2 &End) const;

		void AddObjectToGrid(_Object *Object);
		void AddBlockToGrid(_Block *Block);
		void RemoveObjectFromGrid(_Object *Object);
		void RemoveBlockFromGrid(const _Block *Block);

		void SetAmbientLight(const glm::vec4 &Color) { OldAmbientLight = AmbientLight; AmbientLight = Color; }
		void SetAmbientLightChangePeriod(double Value) { AmbientLightPeriod = Value; AmbientLightTimer = AmbientLightBlendFactor = 0.0; }
		void SetAmbientLightRadius(float Value) { AmbientLightRadius = Value; }

		void RenderFloors();
		void RenderWalls(_Block *ExceptionBlock);
		void RenderObjects(double BlendFactor);
		void RenderGrid(int Spacing, float *Vertices);
		void HighlightBlocks();

		_Tile **&GetTiles() { return Tiles; }

		void AddBlock(_Block *Block);
		void GetSelectedObject(const glm::vec2 &Position, float RadiusSquared, _Spawn **Object, size_t *Index);
		void GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::list<_Spawn *> *SelectedObjects, std::list<std::size_t> *SelectedObjectIndices);
		_Block *GetSelectedBlock(const glm::vec2 &Position);
		void RemoveBlock(const _Block *Block);
		void RemoveObjectSpawns(std::list<std::size_t> &SelectedObjectIndices);

		glm::vec2 GetStartingPositionByCheckpoint(int Level);
		void GetAdjacentTile(const glm::vec2 &Position, float Direction, glm::ivec2 &Coord) const;
		glm::ivec2 GetValidCoord(const glm::ivec2 &Coord) const { return glm::clamp(Coord, glm::ivec2(0), Size - 1); }
		bool CanShootThrough(int IndexX, int IndexY) const;
		void GetTileBounds(const glm::vec2 &Position, float Radius, _TileBounds &TileBounds) const;
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const { return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Size)); }

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
		glm::ivec2 Size;
		const _Atlas *TileAtlas;

		// Objects
		std::vector<_Spawn *> ObjectSpawns;

		// Network
		uint16_t NextObjectID;

		// Stats
		const _Stats *Stats;

	private:

		void InitTiles();
		bool ResolveCircleAABBCollision(const glm::vec2 &Position, float Radius, const glm::vec4 &AABB, bool Resolve, glm::vec2 &Push, bool &DiagonalPush);
		void UpdateShots();

		// Scripting
		_Scripting *Scripting;

		// Blocks
		_Tile **Tiles;
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

// Determines if a tile can be shot through
inline bool _Map::CanShootThrough(int IndexX, int IndexY) const {
	return true;
}

// Returns a bounding rectangle
inline void _Map::GetTileBounds(const glm::vec2 &Position, float Radius, _TileBounds &TileBounds) const {

	// Get tile indices where the bounding rectangle touches
	TileBounds.Start = GetValidCoord(glm::ivec2(Position - Radius));
	TileBounds.End = GetValidCoord(glm::ivec2(Position + Radius));
}
