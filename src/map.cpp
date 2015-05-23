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
#include <objects/shape.h>
#include <objects/item.h>
#include <objects/shot.h>
#include <objects/particle.h>
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
#include <grid.h>
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
#include <map>
#include <stdexcept>
#include <iomanip>
#include <iostream>

const float BLOCK_ADJUST = 0.001f;

// Initialize
_Map::_Map() :
	Filename(""),
	ID(0),
	TileAtlas(nullptr),
	Grid(nullptr),
	NextObjectID(0),
	Stats(nullptr),
	Scripting(nullptr),
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

	// Create uniform grid
	Grid = new _Grid();

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
			File >> Grid->Size.x >> Grid->Size.y;

			// Get tileset
			File >> AtlasPath;

			// Setup tiles
			Grid->InitTiles();
			TilesInitialized = true;

			// Load objects
			size_t ObjectCount;
			File >> ObjectCount;
			for(size_t i = 0; i < ObjectCount; i++) {

				std::string Identifier;
				File >> Identifier;

				// Create object
				_Object *Object = Stats->CreateObject(Identifier, ServerNetwork != nullptr);

				File >> Object->Physics->Position.x >> Object->Physics->Position.y >> Object->Physics->Position.z;
				File >> Object->Shape->HalfWidth.x >> Object->Shape->HalfWidth.y >> Object->Shape->HalfWidth.z;

				std::string TextureIdentifier;
				File >> TextureIdentifier;

				if(ServerNetwork)
					Object->ID = NextObjectID++;

				Object->Map = this;
				if(Object->Render)
					Object->Render->Texture = Assets.Textures[TextureIdentifier];
				if(Object->Physics)
					Object->Physics->LastPosition = Object->Physics->Position;
				AddObject(Object);
				Grid->AddObject(Object);
			}

			// Load tiles
			for(int j = 0; j < Grid->Size.y; j++) {
				for(int i = 0; i < Grid->Size.x; i++) {
					File >> Grid->Tiles[i][j].TextureIndex;
				}
			}

			File.close();
		}
	}
	catch(std::exception &Error) {
		std::cout << Error.what() << std::endl;
	}

	if(!TilesInitialized)
		Grid->InitTiles();

	Scripting = new _Scripting();
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_DEFAULT);

	// Initialize 2d tile rendering
	if(!ServerNetwork) {
		TileAtlas = new _Atlas(Assets.Textures[AtlasPath], glm::ivec2(64, 64), 1);

		GLuint TileVertexCount = 4 * Grid->Size.x * Grid->Size.y;
		GLuint TileFaceCount = 2 * Grid->Size.x * Grid->Size.y;

		TileVertices = new glm::vec4[TileVertexCount];
		TileFaces = new glm::u32vec3[TileFaceCount];

		int FaceIndex = 0;
		int VertexIndex = 0;
		for(int j = 0; j < Grid->Size.y; j++) {
			for(int i = 0; i < Grid->Size.x; i++) {
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

	delete Grid;
	delete Scripting;
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
	Output << Grid->Size.x << " " << Grid->Size.y << '\n';
	Output << TileAtlas->Texture->Identifier << '\n';

	// Objects
	Output << Objects.size() << '\n';
	for(auto Object : Objects) {
		Output << Object->Identifier << " ";
		Output << Object->Physics->Position.x << " ";
		Output << Object->Physics->Position.y << " ";
		Output << Object->Physics->Position.z << " ";
		Output << Object->Shape->HalfWidth.x << " ";
		Output << Object->Shape->HalfWidth.y << " ";
		Output << Object->Shape->HalfWidth.z << " ";
		Output << Object->Render->Texture->Identifier << " ";
		Output << "\n";
	}

	// Write tile map
	for(int j = 0; j < Grid->Size.y; j++) {
		for(int i = 0; i < Grid->Size.x; i++) {
			Output << Grid->Tiles[i][j].TextureIndex << " ";
		}
		Output << '\n';
	}

	Output.close();

	return true;
}

// Check collision with blocks and resolve
bool _Map::CheckCollisions(glm::vec3 &Position, float Radius, std::map<_Object *, bool> &PotentialObjects) {

	// Get AABB of object
	glm::vec3 Start = Position - Radius;
	glm::vec3 End = Position + Radius;

	// Check boundaries
	bool Hit = false;
	if(Start.x < 0) {
		Start.x = Position.x = Radius;
		Hit = true;
	}
	if(Start.y < 0) {
		Start.y = Position.y = Radius;
		Hit = true;
	}
	if(Start.x >= (float)Grid->Size.x) {
		Start.x = Position.x = (float)Grid->Size.x - Radius;
		Hit = true;
	}
	if(End.y >= (float)Grid->Size.y) {
		End.y = Position.y = (float)Grid->Size.y - Radius;
		Hit = true;
	}

	// Iterate twice
	for(int i = 0; i < 2; i++) {

		// Check each block
		bool NoDiag = false;
		std::list<glm::vec3> Pushes;
		for(auto Iterator : PotentialObjects) {
			_Object *Object = Iterator.first;
			glm::vec4 AABB;// = Block->GetAABB();

			bool DiagonalPush = false;
			glm::vec3 Push;
			if(ResolveCircleAABBCollision(Position, Radius, AABB, true, Push, DiagonalPush)) {
				Hit = true;
				Pushes.push_back(Push);

				// If any non-diagonal vectors, flag it
				if(!DiagonalPush)
					NoDiag = true;
			}
		}

		// Resolve collision
		for(auto Push : Pushes) {
			if(!(NoDiag && Push.x != 0 && Push.y != 0)) {
				Position += Push;
			}
		}
	}

	return Hit;
}

// Resolve collision with a tile
bool _Map::ResolveCircleAABBCollision(const glm::vec3 &Position, float Radius, const glm::vec4 &AABB, bool Resolve, glm::vec3 &Push, bool &DiagonalPush) {
	int ClampCount = 0;

	// Get closest point on AABB
	glm::vec3 Point = Position;
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
			glm::vec3 Center(AABB[0] + 0.5f, AABB[1] + 0.5f, 0.0f);
			if(Position.x <= Center.x)
				Push.x = -(AABB[0] - Position.x - Radius);
			else if(Position.x > Center.x)
				Push.x = (AABB[0] - Position.x) + 1 + Radius;
		}
		else {

			// Get push direction
			Push = Position - Point;

			// Get push amount
			float Amount = Radius - glm::length(Push);

			// Scale push vector
			Push = glm::normalize(Push);
			Push *= Amount;

			// Set whether the push is diagonal or not
			DiagonalPush = ClampCount > 1;
		}
	}

	return Hit;
}

// Returns all the objects that fall inside the rectangle
void _Map::GetSelectedObjects(const glm::vec4 &AABB, std::list<_Object *> *SelectedObjects) {

	for(auto Object : Objects) {
		if(!Object->Physics)
			continue;

		if(Object->Physics->Position.x + Object->Shape->HalfWidth[0] >= AABB[0] &&
		   Object->Physics->Position.y + Object->Shape->HalfWidth[0] >= AABB[1] &&
		   Object->Physics->Position.x - Object->Shape->HalfWidth[0] <= AABB[2] &&
		   Object->Physics->Position.y - Object->Shape->HalfWidth[0] <= AABB[3])
			SelectedObjects->push_back(Object);
	}
}

// Returns a starting position by level and player id
glm::vec2 _Map::GetStartingPositionByCheckpoint(int Level) {

	return glm::vec2(0.5f, 0.5f);
}

// Get a valid position within the grid
glm::vec2 _Map::GetValidPosition(const glm::vec2 &Position) const {
	return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Grid->Size));
}

// Draws a grid on the map
void _Map::RenderGrid(int Spacing, float *Vertices) {
	if(Spacing > 0) {
		Graphics.SetColor(COLOR_TWHITE);

		// Build vertical lines
		int Index = 0;
		for(int i = Spacing; i < Grid->Size.x; i += Spacing) {
			Vertices[Index++] = (float)i;
			Vertices[Index++] = 0;
			Vertices[Index++] = (float)i;
			Vertices[Index++] = (float)Grid->Size.y;
		}

		// Build horizontal lines
		for(int i = Spacing; i < Grid->Size.y; i += Spacing) {
			Vertices[Index++] = 0;
			Vertices[Index++] = (float)i;
			Vertices[Index++] = (float)Grid->Size.x;
			Vertices[Index++] = (float)i;
		}

		// Compute number of lines
		int Lines = int((Grid->Size.y-1) / Spacing) + int((Grid->Size.y-1) / Spacing);

		// Draw lines
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, Vertices);
		glDrawArrays(GL_LINES, 0, 2 * Lines);
	}
}

// Draws rectangles around all the blocks
void _Map::HighlightBlocks() {
	/*
	Graphics.SetColor(COLOR_MAGENTA);
	for(auto Block : Blocks) {
		glm::vec4 AABB = Block->GetAABB();
		Graphics.DrawRectangle(glm::vec2(AABB[0], AABB[1]), glm::vec2(AABB[2], AABB[3]));
	}*/
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
	Bounds[0] = glm::clamp(Bounds[0], 0.0f, (float)Grid->Size.x);
	Bounds[1] = glm::clamp(Bounds[1], 0.0f, (float)Grid->Size.y);
	Bounds[2] = glm::clamp(Bounds[2], 0.0f, (float)Grid->Size.x);
	Bounds[3] = glm::clamp(Bounds[3], 0.0f, (float)Grid->Size.y);
	for(int j = Bounds[1]; j < Bounds[3]; j++) {
		for(int i = Bounds[0]; i < Bounds[2]; i++) {
			glm::vec4 TextureCoords = TileAtlas->GetTextureCoords(Grid->Tiles[i][j].TextureIndex);
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

// Render objects
void _Map::RenderObjects(double BlendFactor) {
	RenderList[0].clear();
	RenderList[1].clear();
	RenderList[2].clear();
	RenderList[3].clear();
	RenderList[4].clear();

	// Build render list
	for(auto Object : Objects ) {
		if(Object->Render && Camera && Camera->IsCircleInView(glm::vec2(Object->Physics->Position), Object->Shape->HalfWidth[0])) {
			RenderList[Object->Render->Stats.Layer].push_back(Object);
		}
	}

	// Draw props
	for(auto Iterator : RenderList[4])
		Iterator->Render->Draw3D(BlendFactor);

	// Draw props
	for(auto Iterator : RenderList[3])
		Iterator->Render->Draw3D(BlendFactor);

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

// Update map
void _Map::Update(double FrameTime, uint16_t TimeSteps) {
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

	// Delete objects
	for(auto Object : Objects) {
		Grid->RemoveObject(Object);
		delete Object;
	}
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
	Grid->RemoveObject(Object);
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
