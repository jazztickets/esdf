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
#include <grid.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/shape.h>
#include <objects/shot.h>
#include <constants.h>
#include <stats.h>
#include <map.h>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>

const float BLOCK_ADJUST = 0.001f;

// Constructor
_Grid::_Grid() :
	Size(MAP_SIZE),
	Tiles(nullptr) {
}

// Destructor
_Grid::~_Grid() {

	// Delete tile data
	if(Tiles) {
		for(int i = 0; i < Size.x; i++)
			delete[] Tiles[i];
		delete[] Tiles;
	}
}

// Allocate memory for tile map
void _Grid::InitTiles() {

	// Allocate memory
	Tiles = new _Tile*[Size.x];

	for(int i = 0; i < Size.x; i++)
		Tiles[i] = new _Tile[Size.y];
}

// Adds an object to the collision grid
void _Grid::AddObject(_Object *Object) {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Physics->Position, Object->Shape->Stat.HalfWidth[0], TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			Tiles[i][j].Objects.push_front(Object);
		}
	}
}

// Add block to collision grid
void _Grid::AddBlock(_Block *Block) {

	for(int j = Block->Start.y + BLOCK_ADJUST; j <= (int)(Block->End.y - BLOCK_ADJUST); j++) {
		for(int i = Block->Start.x + BLOCK_ADJUST; i <= (int)(Block->End.x - BLOCK_ADJUST); i++) {
			Tiles[i][j].Blocks.push_back(Block);
		}
	}
}

// Removes an object from the collision grid
void _Grid::RemoveObject(const _Object *Object) {
	if(!Tiles)
		throw std::runtime_error("Tile data uninitialized!");

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Object->Physics->Position, Object->Shape->Stat.HalfWidth[0], TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator = Tiles[i][j].Objects.begin(); Iterator != Tiles[i][j].Objects.end(); ++Iterator) {
				if(*Iterator == Object) {
					Tiles[i][j].Objects.erase(Iterator);
					break;
				}
			}
		}
	}
}

// Remove block from grid
void _Grid::RemoveBlock(const _Block *Block) {
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


// Returns a list of entities that an object is colliding with
void _Grid::CheckEntityCollisionsInGrid(const glm::vec2 &Position, float Radius, const _Object *SkipObject, std::unordered_map<_Object *, bool> &Entities) const {

	// Get the object's bounding rectangle
	_TileBounds TileBounds;
	GetTileBounds(Position, Radius, TileBounds);

	for(int i = TileBounds.Start.x; i <= TileBounds.End.x; i++) {
		for(int j = TileBounds.Start.y; j <= TileBounds.End.y; j++) {
			for(auto Iterator = Tiles[i][j].Objects.begin(); Iterator != Tiles[i][j].Objects.end(); ++Iterator) {
				_Object *Entity = *Iterator;
				if(Entity != SkipObject/* && !Entity->Player->IsDying()*/) {
					float DistanceSquared = glm::distance2(Entity->Physics->Position, Position);
					float RadiiSum = Entity->Shape->Stat.HalfWidth[0] + Radius;

					// Check circle intersection
					if(DistanceSquared < RadiiSum * RadiiSum)
						Entities[Entity] = true;
				}
			}
		}
	}
}

// Checks bullet collisions with objects and walls
void _Grid::CheckBulletCollisions(const _Shot *Shot, _Impact &Impact, bool CheckObjects) const {

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
			for(auto Iterator : Tiles[TileTracer.x][TileTracer.y].Objects) {
				_Object *Object = Iterator;
				float Distance = RayObjectIntersection(Shot->Position, Shot->Direction, Object);
				if(Distance < MinDistance && Distance > 0.0f) {
					Impact.Object = Object;
					MinDistance = Distance;
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

// Determines if two positions are mutually visible
bool _Grid::IsVisible(const glm::vec2 &Start, const glm::vec2 &End) const {
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

// Returns a t value for when a ray intersects a circle
float _Grid::RayObjectIntersection(const glm::vec2 &Origin, const glm::vec2 &Direction, const _Object *Object) const {

	glm::vec2 EMinusC(Origin - Object->Physics->Position);
	float QuantityDDotD = glm::dot(Direction, Direction);
	float QuantityDDotEMC = glm::dot(Direction, EMinusC);
	float Discriminant = QuantityDDotEMC * QuantityDDotEMC - QuantityDDotD * (glm::dot(EMinusC, EMinusC) - Object->Shape->Stat.HalfWidth[0] * Object->Shape->Stat.HalfWidth[0]);
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
