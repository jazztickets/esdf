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
#include <map.h>
#include <ae/network.h>
#include <ae/manager.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/animation.h>
#include <objects/controller.h>
#include <objects/render.h>
#include <objects/shape.h>
#include <objects/zone.h>
#include <objects/item.h>
#include <objects/shot.h>
#include <objects/ai.h>
#include <ae/servernetwork.h>
#include <ae/peer.h>
#include <constants.h>
#include <ae/graphics.h>
#include <ae/texture.h>
#include <ae/atlas.h>
#include <ae/util.h>
#include <ae/assets.h>
#include <ae/camera.h>
#include <ae/mesh.h>
#include <grid.h>
#include <ae/program.h>
#include <packet.h>
#include <scripting.h>
#include <ae/buffer.h>
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
#include <algorithm>

// Initialize
_Map::_Map() :
	Filename(""),
	ID(0),
	TileAtlas(nullptr),
	Grid(nullptr),
	Stats(nullptr),
	Scripting(nullptr),
	TileVertexBufferID(0),
	TileElementBufferID(0),
	TileVertices(nullptr),
	TileFaces(nullptr),
	Camera(nullptr),
	ObjectUpdateCount(0) {

	// Set up render lists
	RenderList.resize(Assets.Layers.size());
	for(const auto &Layer : Assets.Layers)
		RenderList[Layer.second.Layer].Layer = &Layer.second;
}

// Initialize
_Map::_Map(const std::string &Path, const _Stats *Stats, _Manager<_Object> *ObjectManager, NetworkIDType ID, _ServerNetwork *ServerNetwork) : _Map() {
	this->Stats = Stats;
	this->ID = ID;
	this->Filename = Path;
	this->ServerNetwork = ServerNetwork;
	std::string AtlasPath = TEXTURES_TILES + MAP_DEFAULT_TILESET;
	bool TilesInitialized = false;

	// Create uniform grid
	Grid = new _Grid();

	// Load file
	gzifstream File((ASSETS_MAPS_PATH + Path + ".gz").c_str());
	try {
		if(Path != "" && File) {

			_Object *Object = nullptr;
			while(!File.eof() && File.peek() != EOF) {

				// Read chunk type
				char ChunkType;
				File >> ChunkType;

				switch(ChunkType) {
					// Map version
					case 'v': {
						int FileVersion;
						File >> FileVersion;
						if(FileVersion != MAP_FILEVERSION)
							throw std::runtime_error("Level version mismatch: ");
					} break;
					// Map size
					case 'm': {
						File >> Grid->Size.x >> Grid->Size.y;
						Grid->InitTiles();
						TilesInitialized = true;
					} break;
					// Atlas texture
					case 'a': {
						File >> AtlasPath;
					} break;
					// Tile grid data
					case 'g': {
						File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
						for(int j = 0; j < Grid->Size.y; j++) {
							for(int i = 0; i < Grid->Size.x; i++) {
								File >> Grid->Tiles[i][j].TextureIndex;
							}
						}
					} break;
					// Create object
					case 'o': {
						if(!ObjectManager) {
							File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
							break;
						}

						// Add last object
						if(Object)
							Grid->AddObject(Object);

						// Read identifier
						std::string Identifier;
						File >> Identifier;

						// Create object
						Object = ObjectManager->Create();
						Stats->CreateObject(Object, Identifier, ServerNetwork != nullptr);
						Object->Map = this;
						AddObject(Object);
					} break;
					// Object position
					case 'p': {
						if(!ObjectManager) {
							File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
							break;
						}

						File >> Object->Physics->Position.x >> Object->Physics->Position.y >> Object->Physics->Position.z;
						if(Object->Physics)
							Object->Physics->NetworkPosition = Object->Physics->LastPosition = Object->Physics->Position;
					} break;
					// Object shape
					case 's': {
						if(!ObjectManager) {
							File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
							break;
						}

						File >> Object->Shape->HalfWidth.x >> Object->Shape->HalfWidth.y >> Object->Shape->HalfWidth.z;
					} break;
					// Object texture
					case 't': {
						if(!ObjectManager) {
							File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
							break;
						}

						std::string TextureIdentifier;
						File >> TextureIdentifier;
						if(Object->Render)
							Object->Render->Texture = Assets.Textures[TextureIdentifier];
					} break;
					// Zone OnEnter
					case 'e': {
						if(!ObjectManager) {
							File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
							break;
						}

						File.ignore(1);
						std::string OnEnter;
						getline(File, OnEnter);
						if(Object->HasComponent("zone")) {
							_Zone *Zone = (_Zone *)(Object->Components["zone"]);
							Zone->OnEnter = OnEnter;
						}
					} break;
				}
			}

			// Add last object
			if(ObjectManager && Object)
				Grid->AddObject(Object);

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

	// Remove objects
	for(auto &Object : Objects) {
		Grid->RemoveObject(Object);
		Object->Map = nullptr;
	}

	delete Grid;
	delete Scripting;
}

// Saves the level to a file
bool _Map::Save(const std::string &String) {
	if(String == "")
		throw std::runtime_error("Empty file name");

	Filename = String;

	// Open file
	gzofstream Output((ASSETS_MAPS_PATH + Filename + ".gz").c_str());
	if(!Output)
		throw std::runtime_error("Cannot create file: " + Filename);

	Output << std::showpoint << std::fixed << std::setprecision(2);

	// Header
	Output << "v " << MAP_FILEVERSION << '\n';
	Output << "m " << Grid->Size.x << " " << Grid->Size.y << '\n';
	Output << "a " << TileAtlas->Texture->Name << '\n';

	// Objects
	for(auto &Object : Objects) {
		Output << "o " << Object->Identifier << "\n";
		Output << "p " << Object->Physics->Position.x << " " << Object->Physics->Position.y << " " << Object->Physics->Position.z << "\n";
		Output << "s " << Object->Shape->HalfWidth.x << " " << Object->Shape->HalfWidth.y << " " << Object->Shape->HalfWidth.z << "\n";
		if(Object->Render && Object->Render->Texture)
			Output << "t " <<  Object->Render->Texture->Name << "\n";

		if(Object->HasComponent("zone")) {
			_Zone *Zone = (_Zone *)(Object->Components["zone"]);
			if(Zone->OnEnter != "")
				Output << "e " << Zone->OnEnter << "\n";
		}
	}

	// Write tile map
	Output << "g \n";
	for(int j = 0; j < Grid->Size.y; j++) {
		for(int i = 0; i < Grid->Size.x; i++) {
			Output << Grid->Tiles[i][j].TextureIndex << " ";
		}
		Output << '\n';
	}

	Output.close();

	return true;
}

// Returns all the objects that fall inside the rectangle
void _Map::GetSelectedObjects(const glm::vec4 &AABB, std::list<_Object *> &SelectedObjects) {

	for(auto &Object : Objects) {
		if(!Object->Render || !Object->Physics || !Object->Shape)
			continue;

		if(Object->CheckAABB(AABB))
			SelectedObjects.push_back(Object);
	}
}

// Return all objects that are a certain distance from a position
void _Map::QueryObjects(const glm::vec2 &Position, float Radius, std::list<_Object *> &QueriedObjects) {
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
	Graphics.SetColor(COLOR_MAGENTA);
	for(auto &Object : Objects) {
		if(Object->Render && Object->Render->Stats->Layer == 0) {
			glm::vec4 AABB = Object->Shape->GetAABB(Object->Physics->Position);
			Graphics.DrawRectangle3D(glm::vec2(AABB[0], AABB[1]), glm::vec2(AABB[2], AABB[3]), false);
		}
	}
}

// Render the floor
void _Map::RenderFloors() {
	if(!Camera || !TileVertices || !TileFaces)
		return;

	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
	Graphics.SetColor(COLOR_WHITE);

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
void _Map::RenderObjects(double BlendFactor, bool EditorOnly) {
	for(size_t i = 0; i < RenderList.size(); i++)
		RenderList[i].Objects.clear();

	// Build render list
	int Count = 0;
	for(auto &Object : Objects) {
		if(Object->Render && Camera && Object->CheckAABB(Camera->GetAABB())) {
			RenderList[Object->Render->Stats->Layer].Objects.push_back(Object);
			Count++;
		}
	}

	// Render all the objects in each render list
	for(size_t i = 0; i < RenderList.size(); i++) {
		if(EditorOnly || (!EditorOnly && !RenderList[i].Layer->EditorOnly)) {
			Graphics.SetDepthTest(RenderList[i].Layer->DepthTest);
			Graphics.SetDepthMask(RenderList[i].Layer->DepthMask);

			// Draw objects
			for(auto &Iterator : RenderList[i].Objects)
				Iterator->Render->Draw3D(BlendFactor);
		}
	}

	Graphics.SetDepthTest(true);
	Graphics.SetDepthMask(true);
}

// Update map
void _Map::Update(double FrameTime) {
}

// Add object to map and notify peers
void _Map::AddObject(_Object *Object) {

	if(ServerNetwork) {

		// Create packet
		_Buffer Packet;
		Packet.Write<char>(Packet::OBJECT_CREATE);
		Packet.Write<NetworkIDType>(ID);
		Object->NetworkSerialize(Packet);

		// Broadcast to all other peers
		BroadcastPacket(Packet, _Network::RELIABLE);
	}

	// Add to list
	Objects.push_back(Object);
}

// Removes an object from the object list and collision grid
void _Map::RemoveObject(_Object *Object) {

	// Remove pointers to deleted object from other objects
	for(auto &QueryObject : Objects) {
		if(QueryObject->HasComponent("ai")) {
			_Ai *Ai = (_Ai *)(QueryObject->Components["ai"]);
			if(Ai->Target == Object) {
				Ai->Target = nullptr;
			}
		}
	}

	// Notify peers
	if(ServerNetwork) {

		// Create packet
		_Buffer Packet;
		Packet.Write<char>(Packet::OBJECT_DELETE);
		Packet.Write<NetworkIDType>(ID);
		Packet.Write<NetworkIDType>(Object->NetworkID);

		// Send to everyone
		BroadcastPacket(Packet, _Network::RELIABLE);
	}

	// Remove object
	auto Iterator = std::find(Objects.begin(), Objects.end(), Object);
	if(Iterator != Objects.end())
		Objects.erase(Iterator);

	// Remove from collision grid
	Grid->RemoveObject(Object);
}

// Broadcast a packet to all peers in the map
void _Map::BroadcastPacket(_Buffer &Buffer, _Network::SendType Type) {
	if(!ServerNetwork)
		return;

	for(auto &Peer : Peers)
		ServerNetwork->SendPacket(Buffer, Peer, Type, Type == _Network::UNSEQUENCED);
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

	// Create packet
	_Buffer Packet;
	Packet.Write<char>(Packet::OBJECT_LIST);
	Packet.Write<uint16_t>(TimeSteps);
	Packet.Write<NetworkIDType>(Peer->Object->NetworkID);

	// Add place holder for object size
	Packet.Write<NetworkIDType>((NetworkIDType)Objects.size());
	for(auto &Object : Objects) {
		Object->NetworkSerialize(Packet);
	}

	ServerNetwork->SendPacket(Packet, Peer);
}

// Build object update packet for the map
void _Map::SendObjectUpdates(uint16_t TimeSteps) {

	// Create packet
	_Buffer Packet;
	Packet.Write<char>(Packet::OBJECT_UPDATES);
	Packet.Write<NetworkIDType>(ID);
	Packet.Write<uint16_t>(TimeSteps);

	// Write object count
	Packet.Write<NetworkIDType>((NetworkIDType)Objects.size());

	// Iterate over objects
	int Count = 0;
	for(auto &Object : Objects) {
		//if(Object->SendUpdate) {
			Object->NetworkSerializeUpdate(Packet, TimeSteps);
			Object->SendUpdate = false;
			Count++;
		//}
	}

	// Send packet to players in map
	BroadcastPacket(Packet, _Network::UNSEQUENCED);

	//if(Count != ObjectUpdateCount)
	//	throw std::runtime_error("Update count mismatch: " + std::to_string(Count) + " vs " + std::to_string(ObjectUpdateCount));
}
