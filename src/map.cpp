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
#include <map.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/animation.h>
#include <objects/controller.h>
#include <objects/render.h>
#include <objects/item.h>
#include <objects/shot.h>
#include <objects/particle.h>
#include <objects/prop.h>
#include <network/servernetwork.h>
#include <network/peer.h>
#include <constants.h>
#include <graphics.h>
#include <texture.h>
#include <atlas.h>
#include <utils.h>
#include <assets.h>
#include <camera.h>
#include <mesh.h>
#include <particles.h>
#include <program.h>
#include <packet.h>
#include <scripting.h>
#include <buffer.h>
#include <server.h>
#include <stats.h>
#include <zlib/zfstream.h>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>

const float BLOCK_ADJUST = 0.001f;

// Initialize
_Map::_Map() :
	Filename(""),
	ID(0),
	Size(MAP_SIZE),
	TileAtlas(nullptr),
	NextObjectID(0),
	Stats(nullptr),
	Scripting(nullptr),
	Tiles(nullptr),
	TileVertexBufferID(0),
	TileElementBufferID(0),
	TileVertices(nullptr),
	TileFaces(nullptr),
	Camera(nullptr),
	Particles(nullptr),
	AmbientLight(0.0f, 0.0f, 0.0f, 1.0f),
	OldAmbientLight(0.0f, 0.0f, 0.0f, 1.0f),
	AmbientLightRadius(100.0f),
	AmbientLightBlendFactor(1.0),
	AmbientLightPeriod(0.0),
	AmbientLightTimer(0.0),
	ObjectUpdateCount(0) {

}

// Initialize
_Map::_Map(const std::string &Path, const _Stats *Stats, uint8_t ID, _ServerNetwork *ServerNetwork) : _Map() {
	this->Stats = Stats;
	this->ID = ID;
	this->Filename = Path;
	this->ServerNetwork = ServerNetwork;
	std::string AtlasPath = TEXTURES_TILES + MAP_DEFAULT_TILESET;
	bool TilesInitialized = false;

	// Load file
	gzifstream File((ASSETS_MAPS + Path + ".gz").c_str());
	try {
		if(Path != "" && File) {

			// Get file version
			int FileVersion;
			File >> FileVersion;
			if(FileVersion != MAP_FILEVERSION)
				throw std::runtime_error("Level version mismatch: ");

			// Read dimensions
			File >> Size.x >> Size.y;

			// Get tileset
			File >> AtlasPath;

			// Setup tiles
			InitTiles();
			TilesInitialized = true;

			// Load objects
			size_t ObjectCount;
			File >> ObjectCount;
			for(size_t i = 0; i < ObjectCount; i++) {

				// Load Data
				_Spawn *Object = new _Spawn();
				int Type;
				File >> Type >> Object->Identifier >> Object->Position.x >> Object->Position.y;
				ObjectSpawns.push_back(Object);
			}

			// Load props
			size_t PropCount;
			File >> PropCount;
			for(size_t i = 0; i < PropCount; i++) {
				std::string Identifier;
				glm::vec3 Position;

				// Load Data
				File >> Identifier >>
						Position.x >>
						Position.y >>
						Position.z;

				// Create prop
				_Prop *Prop = Stats->CreateProp(Identifier);
				if(!Prop)
					throw std::runtime_error("Unable to load prop: " + Identifier);

				Prop->Position = Position;
				Props.push_back(Prop);
			}

			// Read block size
			size_t BlockCount;
			File >> BlockCount;

			// Load blocks
			for(size_t i = 0; i < BlockCount; i++) {

				_Block *Block = new _Block();

				File >> Block->Start.x >>
						Block->Start.y >>
						Block->Start.z >>
						Block->End.x >>
						Block->End.y >>
						Block->End.z >>
						Block->Collision;

				std::string TextureIdentifier;
				File >> TextureIdentifier;

				if(!ServerNetwork) {
					Block->Texture = Assets.Textures[TextureIdentifier];
				}

				Block->Start = glm::vec3(GetValidPosition(glm::vec2(Block->Start)), Block->Start.z);
				Block->End = glm::vec3(GetValidPosition(glm::vec2(Block->End)), Block->End.z);
				AddBlock(Block);
			}

			// Load tiles
			for(int j = 0; j < Size.y; j++) {
				for(int i = 0; i < Size.x; i++) {
					File >> Tiles[i][j].TextureIndex;
				}
			}

			File.close();
		}
	}
	catch(std::exception &Error) {
		std::cout << Error.what() << std::endl;
	}

	if(!TilesInitialized)
		InitTiles();

	Scripting = new _Scripting();
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_DEFAULT);

	// Initialize 2d tile rendering
	if(!ServerNetwork) {
		TileAtlas = new _Atlas(Assets.Textures[AtlasPath], glm::ivec2(64, 64), 1);

		GLuint TileVertexCount = 4 * Size.x * Size.y;
		GLuint TileFaceCount = 2 * Size.x * Size.y;

		TileVertices = new glm::vec4[TileVertexCount];
		TileFaces = new glm::u32vec3[TileFaceCount];

		int FaceIndex = 0;
		int VertexIndex = 0;
		for(int j = 0; j < Size.y; j++) {
			for(int i = 0; i < Size.x; i++) {
				TileFaces[FaceIndex++] = { VertexIndex + 2, VertexIndex + 1, VertexIndex + 0 };
				TileFaces[FaceIndex++] = { VertexIndex + 2, VertexIndex + 3, VertexIndex + 1 };
				VertexIndex += 4;
			}
		}

		glGenBuffers(1, &TileVertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, TileVertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * TileVertexCount, nullptr, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &TileElementBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TileElementBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::u32vec3) * TileFaceCount, nullptr, GL_DYNAMIC_DRAW);
	}

}

// Shut down
_Map::~_Map() {
	if(!ServerNetwork) {
		delete TileAtlas;
		delete[] TileVertices;
		delete[] TileFaces;
		glDeleteBuffers(1, &TileVertexBufferID);
		glDeleteBuffers(1, &TileElementBufferID);
	}

	// Delete objects
	DeleteObjects();

	// Delete blocks
	for(auto Block : Blocks)
		delete Block;
	Blocks.clear();

	// Delete props
	for(auto Prop : Props)
		delete Prop;
	Props.clear();

	// Delete tile data
	if(Tiles) {
		for(int i = 0; i < Size.x; i++)
			delete[] Tiles[i];
		delete[] Tiles;
	}

	delete Scripting;
}

// Allocate memory for tile map
void _Map::InitTiles() {

	// Allocate memory
	Tiles = new _Tile*[Size.x];

	for(int i = 0; i < Size.x; i++)
		Tiles[i] = new _Tile[Size.y];
}

// Saves the level to a file
bool _Map::Save(const std::string &String) {
	if(String == "")
		throw std::runtime_error("Empty file name");

	Filename = String;

	// Open file
	gzofstream Output((ASSETS_MAPS + Filename + ".gz").c_str());
	if(!Output)
		throw std::runtime_error("Cannot create file: " + Filename);

	Output << std::showpoint << std::fixed << std::setprecision(2);

	// Header
	Output << MAP_FILEVERSION << '\n';
	Output << Size.x << " " << Size.y << '\n';
	Output << TileAtlas->Texture->Identifier << '\n';

	// Objects
	Output << ObjectSpawns.size() << '\n';
	int Type = 0;
	for(size_t i = 0; i < ObjectSpawns.size(); i++) {
		Output << Type << " " << ObjectSpawns[i]->Identifier << " " << ObjectSpawns[i]->Position.x << " " << ObjectSpawns[i]->Position.y << " " << '\n';
	}

	// Props
	Output << Props.size() << '\n';
	for(auto Prop : Props) {
		Output << Prop->Stats.Identifier << " ";
		Output << Prop->Position.x << " ";
		Output << Prop->Position.y << " ";
		Output << Prop->Position.z;
		Output << "\n";
	}

	// Blocks
	Output << Blocks.size() << '\n';
	for(auto Block : Blocks) {
		Output << Block->Start.x << " ";
		Output << Block->Start.y << " ";
		Output << Block->Start.z << " ";
		Output << Block->End.x << " ";
		Output << Block->End.y << " ";
		Output << Block->End.z << " ";
		Output << Block->Collision << " ";
		Output << Block->Texture->Identifier;
		Output << "\n";
	}

	// Write tile map
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			Output << Tiles[i][j].TextureIndex << " ";
		}
		Output << '\n';
	}

	Output.close();

	return true;
}

// Adds an object to the collision grid
void _Map::AddObjectToGrid(_Object *Object) {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Physics->Position, Object->Physics->Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			Tiles[i][j].Objects[Object->GridType].push_front(Object);
		}
	}
}

// Add block to collision grid
void _Map::AddBlockToGrid(_Block *Block) {

	for(int j = Block->Start.y + BLOCK_ADJUST; j <= (int)(Block->End.y - BLOCK_ADJUST); j++) {
		for(int i = Block->Start.x + BLOCK_ADJUST; i <= (int)(Block->End.x - BLOCK_ADJUST); i++) {
			Tiles[i][j].Blocks.push_back(Block);
		}
	}
}

// Removes an object from the collision grid
void _Map::RemoveObjectFromGrid(_Object *Object) {
	if(!Tiles)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Physics->Position, Object->Physics->Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator = Tiles[i][j].Objects[Object->GridType].begin(); Iterator != Tiles[i][j].Objects[Object->GridType].end(); ++Iterator) {
				if(*Iterator == Object) {
					Tiles[i][j].Objects[Object->GridType].erase(Iterator);
					break;
				}
			}
		}
	}
}

// Remove block from grid
void _Map::RemoveBlockFromGrid(const _Block *Block) {
	for(int j = Block->Start.y + BLOCK_ADJUST; j <= (int)(Block->End.y - BLOCK_ADJUST); j++) {
		for(int i = Block->Start.x + BLOCK_ADJUST; i <= (int)(Block->End.x - BLOCK_ADJUST); i++) {
			for(auto Iterator = Tiles[i][j].Blocks.begin(); Iterator != Tiles[i][j].Blocks.end(); ++Iterator) {
				if(*Iterator == Block) {
					Tiles[i][j].Blocks.erase(Iterator);
					break;
				}
			}
		}
	}
}

// Check collision with tiles and resolve
bool _Map::CheckCollisions(const glm::vec2 &TargetPosition, float Radius, glm::vec2 &NewPosition) {
	if(!Tiles)
		throw std::runtime_error("Tile data uninitialized!");

	NewPosition = TargetPosition;
	float Left = NewPosition.x - Radius;
	float Right = NewPosition.x + Radius;
	float Top = NewPosition.y - Radius;
	float Bottom = NewPosition.y + Radius;

	// Check boundaries
	bool Hit = false;
	if(Left < 0) {
		Left = NewPosition.x = Radius;
		Hit = true;
	}
	if(Top < 0) {
		Top = NewPosition.y = Radius;
		Hit = true;
	}
	if(Right >= (float)Size.x) {
		Right = NewPosition.x = (float)Size.x - Radius;
		Hit = true;
	}
	if(Bottom >= (float)Size.y) {
		Bottom = NewPosition.y = (float)Size.y - Radius;
		Hit = true;
	}

	// Check tiles
	int LeftTile = (int)Left;
	int RightTile = (int)Right;
	int TopTile = (int)Top;
	int BottomTile = (int)Bottom;

	int PushCount = 0;
	glm::vec2 Pushes[4];
	bool NoDiag = false;
	for(int i = LeftTile; i <= RightTile; i++) {
		for(int j = TopTile; j <= BottomTile; j++) {
			if(!Tiles[i][j].CanWalk()) {

				bool DiagonalPush = false;
				if(CheckTileCollision(NewPosition, Radius, (float)i, (float)j, true, Pushes[PushCount], DiagonalPush)) {
					Hit = true;
					PushCount++;

					// If any non-diagonal vectors, flag it
					if(!DiagonalPush)
						NoDiag = true;
				}
			}
		}
	}

	// Resolve collision
	for(int i = 0; i < PushCount; i++) {
		if(!(NoDiag && Pushes[i].x != 0 && Pushes[i].y != 0)) {
			NewPosition += Pushes[i];
		}
	}

	return Hit;
}

// Resolve collision with a tile
bool _Map::CheckTileCollision(const glm::vec2 &Position, float Radius, float X, float Y, bool Resolve, glm::vec2 &Push, bool &DiagonalPush) {
	float AABB[4] = { X, Y, X + 1, Y + 1 };
	int ClampCount = 0;

	// Get closest point on AABB
	glm::vec2 Point = Position;
	if(Point.x < AABB[0]) {
		Point.x = AABB[0];
		ClampCount++;
	}
	if(Point.y < AABB[1]) {
		Point.y = AABB[1];
		ClampCount++;
	}
	if(Point.x > AABB[2]) {
		Point.x = AABB[2];
		ClampCount++;
	}
	if(Point.y > AABB[3]) {
		Point.y = AABB[3];
		ClampCount++;
	}

	// Test circle collision with point
	float DistanceSquared = glm::distance2(Point, Position);
	bool Hit = DistanceSquared < Radius * Radius;

	// Push object out
	if(Hit && Resolve) {

		// Check if object is inside the AABB
		if(ClampCount == 0) {
			glm::vec2 Center(X + 0.5f, Y + 0.5f);
			if(Position.x <= Center.x)
				Push.x = -(X - Position.x - Radius);
			else if(Position.x > Center.x)
				Push.x = (X - Position.x) + 1 + Radius;
		}
		else {

			// Get push direction
			Push = Position - Point;

			// Get push amount
			float Amount = Radius - glm::length(Push);

			// Scale push vector
			Push = glm::normalize(Push);
			Push *= Amount;

			// Set whether the push is diagnol or not
			DiagonalPush = ClampCount > 1;
		}
	}

	return Hit;
}

// Checks for collisions with an object in the collision grid
_Object *_Map::CheckCollisionsInGrid(const glm::vec2 &Position, float Radius, int GridType, const _Object *SkipObject) const {
	if(!Tiles)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator : Tiles[i][j].Objects[GridType]) {
				if(Iterator != SkipObject) {
					float DistanceSquared = glm::distance2(Iterator->Physics->Position, Position);
					float RadiiSum = Iterator->Physics->Radius + Radius;

					// Check circle intersection
					if(DistanceSquared < RadiiSum * RadiiSum) {
						return Iterator;
					}
				}
			}
		}
	}

	return nullptr;
}

// Returns a list of entities that an object is colliding with
void _Map::CheckEntityCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, std::list<_Object *> &Entities) const {
	if(!Tiles)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(int k = 0; k < 2; k++) {
				for(auto Iterator = Tiles[i][j].Objects[k].begin(); Iterator != Tiles[i][j].Objects[k].end(); ++Iterator) {
					_Object *Entity = *Iterator;
					if(Entity != SkipObject/* && !Entity->Player->IsDying()*/) {
						float DistanceSquared = glm::distance2(Entity->Physics->Position, Position);
						float RadiiSum = Entity->Physics->Radius + Radius;

						// Check circle intersection
						if(DistanceSquared < RadiiSum * RadiiSum)
							Entities.push_back(Entity);
					}
				}
			}
		}
	}
}

// Determines what adjacent square the object is facing
void _Map::GetAdjacentTile(const glm::vec2 &Position, float Direction, glm::ivec2 &Coord) const {

	// Check direction
	if(Direction > 45.0f && Direction < 135.0f)
		Coord = GetValidCoord(glm::ivec2(Position.x + 1.0f, Position.y));
	else if(Direction >= 135.0f && Direction < 225.0f)
		Coord = GetValidCoord(glm::ivec2(Position.x, Position.y + 1.0f));
	else if(Direction >= 225.0f && Direction < 315.0f)
		Coord = GetValidCoord(glm::ivec2(Position.x - 1.0f, Position.y));
	else
		Coord = GetValidCoord(glm::ivec2(Position.x, Position.y - 1.0f));
}

// Checks bullet collisions with objects and walls
void _Map::CheckBulletCollisions(const _Shot *Shot, _Impact &Impact, bool CheckObjects) const {
	if(!Tiles)
		throw std::runtime_error("Tile data uninitialized!");

	// Find slope
	float Slope = Shot->Direction.y / Shot->Direction.x;

	// Find starting tile
	glm::ivec2 TileTracer = GetValidCoord(Shot->Position);

	// Check x direction
	int TileIncrementX, FirstBoundaryTileX;
	if(Shot->Direction.x < 0) {
		FirstBoundaryTileX = TileTracer.x;
		TileIncrementX = -1;
	}
	else {
		FirstBoundaryTileX = TileTracer.x + 1;
		TileIncrementX = 1;
	}

	// Check y direction
	int TileIncrementY, FirstBoundaryTileY;
	if(Shot->Direction.y < 0) {
		FirstBoundaryTileY = TileTracer.y;
		TileIncrementY = -1;
	}
	else {
		FirstBoundaryTileY = TileTracer.y + 1;
		TileIncrementY = 1;
	}

	// Find ray direction ratios
	glm::vec2 Ratio(1.0f / Shot->Direction.x, 1.0f / Shot->Direction.y);

	// Calculate increments
	glm::vec2 Increment(TileIncrementX * Ratio.x, TileIncrementY * Ratio.y);

	// Get starting positions
	glm::vec2 Tracer((FirstBoundaryTileX - Shot->Position.x) * Ratio.x, (FirstBoundaryTileY - Shot->Position.y) * Ratio.y);

	// Traverse tiles
	if(CheckObjects)
		Impact.Object = nullptr;

	float MinDistance = HUGE_VAL;
	bool EndedOnX = false;
	while(TileTracer.x >= 0 && TileTracer.y >= 0 && TileTracer.x < Size.x && TileTracer.y < Size.y && CanShootThrough(TileTracer.x, TileTracer.y)) {

		// Check for object intersections
		if(CheckObjects) {
			for(auto Iterator : Tiles[TileTracer.x][TileTracer.y].Objects[Shot->TargetFilter]) {
				_Object *Object = Iterator;
				if(1/*!Object->Player->IsDying()*/) {
					float Distance = RayObjectIntersection(Shot->Position, Shot->Direction, Object);
					if(Distance < MinDistance && Distance > 0.0f) {
						Impact.Object = Object;
						MinDistance = Distance;
					}
				}
			}
		}

		// Determine which direction needs an update
		if(Tracer.x < Tracer.y) {
			Tracer.x += Increment.x;
			TileTracer.x += TileIncrementX;
			EndedOnX = true;
		}
		else {
			Tracer.y += Increment.y;
			TileTracer.y += TileIncrementY;
			EndedOnX = false;
		}
	}

	// An object was hit
	if(CheckObjects && Impact.Object != nullptr) {
		Impact.Type = _Impact::OBJECT;
		Impact.Position = Shot->Direction * MinDistance + Shot->Position;
		Impact.Distance = glm::length(Impact.Position - Shot->Position);
		return;
	}

	// Determine which side has hit
	glm::vec2 WallHitPosition;
	if(EndedOnX) {

		// Get correct side of the wall
		FirstBoundaryTileX = Shot->Direction.x < 0 ? TileTracer.x+1 : TileTracer.x;
		float WallBoundary = FirstBoundaryTileX - Shot->Position.x;

		// Determine hit position
		WallHitPosition.x = WallBoundary;
		WallHitPosition.y = WallBoundary * Slope;
	}
	else {

		// Get correct side of the wall
		FirstBoundaryTileY = Shot->Direction.y < 0 ? TileTracer.y+1 : TileTracer.y;
		float WallBoundary = FirstBoundaryTileY - Shot->Position.y;

		// Determine hit position
		WallHitPosition.x = WallBoundary / Slope;
		WallHitPosition.y = WallBoundary;
	}

	Impact.Type = _Impact::WALL;
	Impact.Position = WallHitPosition + Shot->Position;
	Impact.Distance = glm::length(Impact.Position - Shot->Position);
	if(CheckObjects)
		Impact.Object = nullptr;
}

// Returns a t value for when a ray intersects a circle
float _Map::RayObjectIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction, const _Object *Object) const {

	glm::vec2 EMinusC(Origin - Object->Physics->Position);
	float QuantityDDotD = glm::dot(Direction, Direction);
	float QuantityDDotEMC = glm::dot(Direction, EMinusC);
	float Discriminant = QuantityDDotEMC * QuantityDDotEMC - QuantityDDotD * (glm::dot(EMinusC, EMinusC) - Object->Physics->Radius * Object->Physics->Radius);
	if(Discriminant >= 0) {
		float ProductRayOMinusC = glm::dot(Direction * -1.0f, EMinusC);
		float SqrtDiscriminant = sqrt(Discriminant);

		float TMinus = (ProductRayOMinusC - SqrtDiscriminant) / QuantityDDotD;
		if(TMinus > 0)
			return TMinus;
		else
			return (ProductRayOMinusC + SqrtDiscriminant) / QuantityDDotD;
	}
	else
		return HUGE_VAL;
}

// Determines if two positions are mutually visible
bool _Map::IsVisible(const glm::vec2 &Start, const glm::vec2 &End) const {
	glm::vec2 Direction, Tracer, Increment, Ratio;
	int TileIncrementX, TileIncrementY, FirstBoundaryTileX, FirstBoundaryTileY, TileTracerX, TileTracerY;

	// Find starting and ending tiles
	glm::ivec2 StartTile = GetValidCoord(glm::ivec2(Start));
	glm::ivec2 EndTile = GetValidCoord(glm::ivec2(End));

	// Get direction
	Direction = End - Start;

	// Check degenerate cases
	if(!CanShootThrough(StartTile.x, StartTile.y) || !CanShootThrough(EndTile.x, EndTile.y))
		return false;

	// Only need to check vertical tiles
	if(StartTile.x == EndTile.x) {

		// Check degenerate cases
		if(StartTile.y == EndTile.y)
			return true;

		// Check direction
		if(Direction.y < 0) {
			for(int i = EndTile.y; i <= StartTile.y; i++) {
				if(!CanShootThrough(StartTile.x, i))
					return false;
			}
		}
		else {
			for(int i = StartTile.y; i <= EndTile.y; i++) {
				if(!CanShootThrough(StartTile.x, i))
					return false;
			}
		}
		return true;
	}
	else if(StartTile.y == EndTile.y) {

		// Check direction
		if(Direction.x < 0) {
			for(int i = EndTile.x; i <= StartTile.x; i++) {
				if(!CanShootThrough(i, StartTile.y))
					return false;
			}
		}
		else {
			for(int i = StartTile.x; i <= EndTile.x; i++) {
				if(!CanShootThrough(i, StartTile.y))
					return false;
			}
		}
		return true;
	}

	// Check x direction
	if(Direction.x < 0) {
		FirstBoundaryTileX = StartTile.x;
		TileIncrementX = -1;
	}
	else {
		FirstBoundaryTileX = StartTile.x + 1;
		TileIncrementX = 1;
	}

	// Check y direction
	if(Direction.y < 0) {
		FirstBoundaryTileY = StartTile.y;
		TileIncrementY = -1;
	}
	else {
		FirstBoundaryTileY = StartTile.y + 1;
		TileIncrementY = 1;
	}

	// Find ray direction ratios
	Ratio.x = 1.0f / Direction.x;
	Ratio.y = 1.0f / Direction.y;

	// Calculate increments
	Increment.x = TileIncrementX * Ratio.x;
	Increment.y = TileIncrementY * Ratio.y;

	// Get starting positions
	Tracer.x = (FirstBoundaryTileX - Start.x) * Ratio.x;
	Tracer.y = (FirstBoundaryTileY - Start.y) * Ratio.y;

	// Starting tiles
	TileTracerX = StartTile.x;
	TileTracerY = StartTile.y;

	// Traverse tiles
	while(true) {

		// Check for walls
		if(TileTracerX < 0 || TileTracerY < 0 || TileTracerX >= Size.x || TileTracerY >= Size.y || !CanShootThrough(TileTracerX, TileTracerY))
			return false;

		// Determine which direction needs an update
		if(Tracer.x < Tracer.y) {
			Tracer.x += Increment.x;
			TileTracerX += TileIncrementX;
		}
		else {
			Tracer.y += Increment.y;
			TileTracerY += TileIncrementY;
		}

		// Exit condition
		if((Direction.x < 0 && TileTracerX < EndTile.x)
			|| (Direction.x > 0 && TileTracerX > EndTile.x)
			|| (Direction.y < 0 && TileTracerY < EndTile.y)
			|| (Direction.y > 0 && TileTracerY > EndTile.y))
			break;
	}

	return true;
}

// Return an object at a given position
void _Map::GetSelectedObject(const glm::vec2 &Position, float RadiusSquared, _Spawn **Object, size_t *Index) {

	for(size_t i = 0; i < ObjectSpawns.size(); i++) {

		// Circle test
		if(glm::distance2(ObjectSpawns[i]->Position, Position) < RadiusSquared) {
			*Object = ObjectSpawns[i];
			*Index = i;
			return;
		}
	}

	*Object = nullptr;
}

// Returns all the objects that fall inside the rectangle
void _Map::GetSelectedObjects(const glm::vec2 &Start, const glm::vec2 &End, std::list<_Spawn *> *SelectedObjects, std::list<size_t> *SelectedObjectIndices) {

	glm::vec2 StartPoint, EndPoint;
	if(End.x < Start.x) {
		StartPoint.x = End.x;
		EndPoint.x = Start.x;
	}
	else {
		StartPoint.x = Start.x;
		EndPoint.x = End.x;
	}

	if(End.y < Start.y) {
		StartPoint.y = End.y;
		EndPoint.y = Start.y;
	}
	else {
		StartPoint.y = Start.y;
		EndPoint.y = End.y;
	}

	for(size_t i = 0; i < ObjectSpawns.size(); i++) {
		if(ObjectSpawns[i]->Position.x > StartPoint.x && ObjectSpawns[i]->Position.y > StartPoint.y && ObjectSpawns[i]->Position.x <= EndPoint.x && ObjectSpawns[i]->Position.y <= EndPoint.y) {
			SelectedObjects->push_back(ObjectSpawns[i]);
			SelectedObjectIndices->push_back(i);
		}
	}
}

// Removes a block from the list
void _Map::RemoveBlock(const _Block *Block) {
	for(auto Iterator = Blocks.begin(); Iterator != Blocks.end(); ++Iterator) {
		if(Block == *Iterator) {
			Blocks.erase(Iterator);
			RemoveBlockFromGrid(Block);
			delete Block;
			return;
		}
	}
}

// Removes object spawns from the list
void _Map::RemoveObjectSpawns(std::list<size_t> &SelectedObjectIndices) {
	SelectedObjectIndices.sort();
	for(std::list<size_t>::reverse_iterator Iterator = SelectedObjectIndices.rbegin(); Iterator != SelectedObjectIndices.rend(); ++Iterator)
		ObjectSpawns.erase(ObjectSpawns.begin() + *Iterator);
}

// Return the block at a given position
_Block *_Map::GetSelectedBlock(const glm::vec2 &Position) {

	for(auto Iterator = Blocks.rbegin(); Iterator != Blocks.rend(); ++Iterator) {
		_Block *Block = *Iterator;
		if(Position.x >= Block->Start.x && Position.y >= Block->Start.y && Position.x <= Block->End.x && Position.y <= Block->End.y)
			return Block;
	}

	return nullptr;
}

// Returns a starting position by level and player id
glm::vec2 _Map::GetStartingPositionByCheckpoint(int Level) {

	return glm::vec2(0.5f, 0.5f);
}

// Draws a grid on the map
void _Map::RenderGrid(int Spacing, float *Vertices) {
	if(Spacing > 0) {
		Graphics.SetColor(COLOR_TWHITE);

		// Build vertical lines
		int Index = 0;
		for(int i = Spacing; i < Size.x; i += Spacing) {
			Vertices[Index++] = (float)i;
			Vertices[Index++] = 0;
			Vertices[Index++] = (float)i;
			Vertices[Index++] = (float)Size.y;
		}

		// Build horizontal lines
		for(int i = Spacing; i < Size.y; i += Spacing) {
			Vertices[Index++] = 0;
			Vertices[Index++] = (float)i;
			Vertices[Index++] = (float)Size.x;
			Vertices[Index++] = (float)i;
		}

		// Compute number of lines
		int Lines = int((Size.y-1) / Spacing) + int((Size.y-1) / Spacing);

		// Draw lines
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, Vertices);
		glDrawArrays(GL_LINES, 0, 2 * Lines);
	}
}

// Draws rectangles around all the blocks
void _Map::HighlightBlocks() {
	for(auto Block : Blocks) {
		Graphics.DrawRectangle(glm::vec2(Block->Start), glm::vec2(Block->End), COLOR_MAGENTA);
	}
}

// Add a block to the list
void _Map::AddBlock(_Block *Block) {
	Blocks.push_back(Block);
	AddBlockToGrid(Block);
}

// Render the floor
void _Map::RenderFloors() {
	if(!Camera || !TileVertices || !TileFaces)
		return;

	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

	int VertexIndex = 0;
	int FaceIndex = 0;
	glm::vec4 Bounds = Camera->GetAABB();
	Bounds[0] = glm::clamp(Bounds[0], 0.0f, (float)Size.x);
	Bounds[1] = glm::clamp(Bounds[1], 0.0f, (float)Size.y);
	Bounds[2] = glm::clamp(Bounds[2], 0.0f, (float)Size.x);
	Bounds[3] = glm::clamp(Bounds[3], 0.0f, (float)Size.y);
	for(int j = Bounds[1]; j < Bounds[3]; j++) {
		for(int i = Bounds[0]; i < Bounds[2]; i++) {
			glm::vec4 TextureCoords = TileAtlas->GetTextureCoords(Tiles[i][j].TextureIndex);
			TileVertices[VertexIndex++] = { i + 0.0f, j + 0.0f, TextureCoords[0], TextureCoords[1] };
			TileVertices[VertexIndex++] = { i + 1.0f, j + 0.0f, TextureCoords[2], TextureCoords[1] };
			TileVertices[VertexIndex++] = { i + 0.0f, j + 1.0f, TextureCoords[0], TextureCoords[3] };
			TileVertices[VertexIndex++] = { i + 1.0f, j + 1.0f, TextureCoords[2], TextureCoords[3] };

			FaceIndex += 2;
		}
	}

	GLsizeiptr VertexBufferSize = VertexIndex * sizeof(glm::vec4);
	GLsizeiptr ElementBufferSize = FaceIndex * sizeof(glm::u32vec3);

	glBindTexture(GL_TEXTURE_2D, TileAtlas->Texture->ID);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, TileVertexBufferID);
	glBufferSubData(GL_ARRAY_BUFFER, 0, VertexBufferSize, TileVertices);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)sizeof(glm::vec2));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TileElementBufferID);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ElementBufferSize, TileFaces);
	glDrawElements(GL_TRIANGLES, FaceIndex * 3, GL_UNSIGNED_INT, 0);

	Graphics.DirtyState();
}

// Render the walls
void _Map::RenderWalls(_Block *ExceptionBlock) {
	if(!Camera)
		return;

	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	Graphics.SetVBO(VBO_CUBE);

	// Draw walls
	for(auto Block : Blocks) {
		if(Block == ExceptionBlock)
			continue;

		bool Draw = true;
		if(Block->Start.z >= 0) {
			float Bounds[4] = { Block->Start.x, Block->Start.y, Block->End.x, Block->End.y };
			Draw = Camera->IsAABBInView(Bounds);
		}

		if(Draw)
			Graphics.DrawCube(Block->Start, Block->End - Block->Start, Block->Texture);
	}
}

// Render objects
void _Map::RenderObjects(double BlendFactor) {
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	Graphics.SetDepthMask(false);

	// Draw items
	for(auto Iterator : RenderList[0])
		Iterator->Render->Draw3D(BlendFactor);

	// Draw player
	for(auto Iterator : RenderList[1])
		Iterator->Render->Draw3D(BlendFactor);

	// Draw monsters
	for(auto Iterator : RenderList[2])
		Iterator->Render->Draw3D(BlendFactor);

	Graphics.SetDepthMask(true);
}

// Render the props
void _Map::RenderProps() {
	if(!Camera)
		return;

	// Draw props
	for(auto Prop : Props) {
		if(Camera->IsCircleInView(glm::vec2(Prop->Position), Prop->Stats.Radius))
			Prop->Render();
	}
}

// Update map
void _Map::Update(double FrameTime, uint16_t TimeSteps) {
	RenderList[0].clear();
	RenderList[1].clear();
	RenderList[2].clear();
	ObjectUpdateCount = 0;

	// Update objects
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		_Object *Object = *Iterator;

		// Update the object
		Object->Update(FrameTime, TimeSteps);

		// Delete old objects
		if(Object->Deleted) {
			if(Object->Peer) {
				RemovePeer(Object->Peer);
			}
			RemoveObject(Object);

			delete Object;
			Iterator = Objects.erase(Iterator);
		}
		else {
			if(Object->SendUpdate)
				ObjectUpdateCount++;

			// TODO IsCircleInView should be called after Set3DProjection
			if(Object->Render && Camera && Camera->IsCircleInView(Object->Physics->Position, Object->Render->Scale)) {
				RenderList[Object->Render->Layer].push_back(Object);
			}

			++Iterator;
		}
	}

	UpdateShots();

	if(AmbientLightPeriod > 0 && AmbientLightTimer <= AmbientLightPeriod) {
		AmbientLightBlendFactor = AmbientLightTimer / AmbientLightPeriod;
		AmbientLightTimer += FrameTime;
	}
	else
		AmbientLightBlendFactor = 1.0;
}

// Apply shots
void _Map::UpdateShots() {

	/*
	for(int i = 0; i < BulletsShot; i++) {
		HitInformation.Type = HIT_NONE;

		switch(HitInformation.Type) {
			case HIT_NONE:
			break;
			case HIT_WALL:
			break;
			case HIT_OBJECT:
			break;
		}
	}
	*/

	// Iterate through shots
	for(auto Shot : Shots) {
		/*
		// Check for collisions
		_Impact Impact;
		CheckBulletCollisions(Shot, Impact, true);

		switch(Impact.Type) {
			case _Impact::NONE: {
			} break;
			case _Impact::OBJECT: {
			} break;
			case _Impact::WALL: {
			} break;
		}
		*/
		delete Shot;
	}

	Shots.clear();
}

// Delete all objects
void _Map::DeleteObjects() {

	// Remove object spawns
	for(size_t i = 0; i < ObjectSpawns.size(); i++)
		delete ObjectSpawns[i];
	ObjectSpawns.clear();

	// Delete objects
	for(auto Iterator : Objects)
		delete Iterator;
	Objects.clear();
}

// Removes an object from the object list and collision grid
void _Map::RemoveObject(_Object *Object) {

	// Notify peers
	if(ServerNetwork) {
		_Buffer Buffer;
		Buffer.Write<char>(Packet::OBJECT_DELETE);
		Buffer.Write<uint8_t>(ID);
		Buffer.Write<uint16_t>(Object->ID);
		for(auto Peer : Peers) {
			ServerNetwork->SendPacket(Buffer, Peer, _Network::RELIABLE);
		}
	}

	// Remove from collision grid
	RemoveObjectFromGrid(Object);
}

// Add shot to map
void _Map::AddShot(const _Shot *Shot) {
	Shots.push_back(Shot);
}

// Remove a peer
void _Map::RemovePeer(const _Peer *Peer) {
	for(auto Iterator = Peers.begin(); Iterator != Peers.end(); ++Iterator) {
		if(*Iterator == Peer) {
			Peers.erase(Iterator);
			return;
		}
	}
}

// Send the object list to a peer
void _Map::SendObjectList(_Object *Player, uint16_t TimeSteps) {
	const _Peer *Peer = Player->Peer;
	if(!Peer)
		return;

	// Send map object list to peer
	_Buffer Buffer;
	Buffer.Write<char>(Packet::OBJECT_LIST);
	Buffer.Write<uint8_t>(ID);
	Buffer.Write<uint16_t>(TimeSteps);
	Buffer.Write<uint16_t>(Player->ID);
	BuildObjectList(Buffer);
	ServerNetwork->SendPacket(Buffer, Peer);
}

// Build a complete list of objects in the map
void _Map::BuildObjectList(_Buffer &Buffer) {

	// Add place holder for object size
	Buffer.Write<uint16_t>(Objects.size());
	for(auto Object : Objects) {

		Object->NetworkSerialize(Buffer);
	}
}

// Build object update packet for the map
void _Map::BuildObjectUpdate(_Buffer &Buffer, uint16_t TimeSteps) {

	// Check for peers in the map
	if(Peers.size() == 0)
		return;

	// Write object updates
	Buffer.Write<uint16_t>(ObjectUpdateCount);
	int Count = 0;
	for(auto Object : Objects) {
		if(Object->SendUpdate) {
			Object->NetworkSerializeUpdate(Buffer, TimeSteps);
			Object->SendUpdate = false;
			Count++;
		}
	}

	if(Count != ObjectUpdateCount)
		throw std::runtime_error("Update count mismatch: " + std::to_string(Count) + " vs " + std::to_string(ObjectUpdateCount));
}

// Update the objects from a packet
void _Map::UpdateObjectsFromBuffer(_Buffer &Buffer, uint16_t TimeSteps) {
	uint16_t ObjectCount = Buffer.Read<uint16_t>();
	for(uint16_t i = 0; i < ObjectCount; i++) {
		uint16_t ID = Buffer.Read<uint16_t>();
		_Object *Object = GetObjectByID(ID);
		if(Object)
			Object->NetworkUnserializeUpdate(Buffer, TimeSteps);
	}
}

// Find an object by id
_Object *_Map::GetObjectByID(uint16_t ObjectID) {
	for(auto Object : Objects) {
		if(Object->ID == ObjectID)
			return Object;
	}

	return nullptr;
}
