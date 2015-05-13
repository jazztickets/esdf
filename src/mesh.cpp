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
#include <mesh.h>
#include <utils.h>
#include <stdexcept>
#include <fstream>
#include <limits>
#include <map>

// Constructor
_Mesh::_Mesh(const std::string &Path) :
	Identifier(Path),
	IndexCount(0),
	Flags(0),
	Version(0),
	VertexBufferID(-1),
	ElementBufferID(-1) {

	// Open file
	std::ifstream File(Path.c_str(), std::ios_base::binary);
	if(!File)
		throw std::runtime_error("Failed to open .mesh file for reading: " + Path);

	// Read header
	File.read((char *)&Version, sizeof(Version));
	File.read((char *)&Flags, sizeof(Flags));

	// Read counts
	uint32_t VertexCount;
	File.read((char *)&VertexCount, sizeof(VertexCount));
	File.read((char *)&IndexCount, sizeof(IndexCount));

	// Prepare storage
	std::vector<GLuint> PackedIndices;
	std::vector<_PackedVertex> PackedVertices;
	PackedVertices.resize(VertexCount);
	PackedIndices.resize(IndexCount);

	// Read data
	File.read((char *)PackedVertices.data(), sizeof(_PackedVertex) * VertexCount);
	File.read((char *)PackedIndices.data(), sizeof(GLuint) * IndexCount);

	File.close();

	// Create vertex buffer
	glGenBuffers(1, &VertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(_PackedVertex) * PackedVertices.size(), PackedVertices.data(), GL_STATIC_DRAW);

	// Create index buffer
	glGenBuffers(1, &ElementBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * PackedIndices.size(), PackedIndices.data(), GL_STATIC_DRAW);
}

// Destructor
_Mesh::~_Mesh() {
	if(VertexBufferID)
		glDeleteBuffers(1, &VertexBufferID);

	if(ElementBufferID)
		glDeleteBuffers(1, &ElementBufferID);
}

// Load .obj file
void _Mesh::ConvertOBJ(const std::string &Path) {

	// Open file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Failed to open .obj file for reading: " + Path);

	// Clear mesh
	uint32_t Flags = 0;
	uint8_t Version = 0;
	std::vector<GLuint> PackedIndices;
	std::vector<_PackedVertex> PackedVertices;

	// Read file
	std::vector<GLuint> VertexIndices;
	std::vector<GLuint> UVIndices;
	std::vector<GLuint> NormalIndices;
	std::vector<glm::vec3> Vertices;
	std::vector<glm::vec2> UVs;
	std::vector<glm::vec3> Normals;
	while(!File.eof()) {
		char FirstChar = File.get();
		if(File.eof())
			break;

		// Handle first char
		switch(FirstChar) {
			case 'v': {
				char NextChar = File.peek();
				switch(NextChar) {
					case 't': {
						File.get();
						glm::vec2 UV;
						File >> UV.s >> UV.t;
						UVs.push_back(UV);

						Flags |= HAS_UVS;
					} break;
					case 'n': {
						File.get();
						glm::vec3 Normal;
						File >> Normal.x >> Normal.y >> Normal.z;
						Normals.push_back(Normal);

						Flags |= HAS_NORMALS;
					} break;
					default: {
						glm::vec3 Vertex;
						File >> Vertex.x >> Vertex.y >> Vertex.z;

						Vertices.push_back(Vertex);
					} break;
				}
			} break;
			case 'f': {
				GLuint VertexIndex[3];

				// Blender exports CW, so load them in reverse
				if(Flags & HAS_UVS) {
					GLuint UVIndex[3];
					char Dummy;

					if(Flags & HAS_NORMALS) {
						GLuint NormalIndex[3];
						File	>> VertexIndex[2] >> Dummy >> UVIndex[2] >> Dummy >> NormalIndex[2]
								>> VertexIndex[1] >> Dummy >> UVIndex[1] >> Dummy >> NormalIndex[1]
								>> VertexIndex[0] >> Dummy >> UVIndex[0] >> Dummy >> NormalIndex[0];

						NormalIndices.push_back(NormalIndex[0]);
						NormalIndices.push_back(NormalIndex[1]);
						NormalIndices.push_back(NormalIndex[2]);
					}
					else {
						File >> VertexIndex[2] >> Dummy >> UVIndex[2] >> VertexIndex[1] >> Dummy >> UVIndex[1] >> VertexIndex[0] >> Dummy >> UVIndex[0];
					}

					UVIndices.push_back(UVIndex[0]);
					UVIndices.push_back(UVIndex[1]);
					UVIndices.push_back(UVIndex[2]);
				}
				else {
					File >> VertexIndex[2] >> VertexIndex[1] >> VertexIndex[0];
				}

				VertexIndices.push_back(VertexIndex[0]);
				VertexIndices.push_back(VertexIndex[1]);
				VertexIndices.push_back(VertexIndex[2]);
			} break;
		}

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	// Pack vertices
	std::map<_PackedVertex, GLuint> PackedVertexMap;
	std::map<_PackedVertex, GLuint>::iterator PackedVertexMapIterator;
	for(size_t i = 0; i < VertexIndices.size(); i++) {

		// Create packed vertex
		_PackedVertex PackedVertex;
		PackedVertex.Position = Vertices[VertexIndices[i] - 1];
		if(Flags & HAS_UVS)
			PackedVertex.UV = UVs[UVIndices[i] - 1];

		if(Flags & HAS_NORMALS)
			PackedVertex.Normal = Normals[NormalIndices[i] - 1];

		// Search for existing vertex
		PackedVertexMapIterator = PackedVertexMap.find(PackedVertex);
		if(PackedVertexMapIterator == PackedVertexMap.end()) {
			PackedVertices.push_back(PackedVertex);
			PackedVertexMap[PackedVertex] = PackedVertices.size() - 1;
			PackedIndices.push_back(PackedVertices.size() - 1);
		}
		else {
			PackedIndices.push_back(PackedVertexMapIterator->second);
		}
	}

	File.close();

	// Open file
	std::string OutPath = RemoveExtension(Path) + ".mesh";
	std::ofstream OutFile(OutPath.c_str(), std::ios_base::binary);
	if(!OutFile)
		throw std::runtime_error("Failed to open .mesh file for writing: " + OutPath);

	// Write header
	OutFile.write((char *)&Version, sizeof(Version));
	OutFile.write((char *)&Flags, sizeof(Flags));

	// Write counts
	uint32_t VertexCount = (uint32_t)PackedVertices.size();
	uint32_t IndexCount = (uint32_t)PackedIndices.size();
	OutFile.write((char *)&VertexCount, sizeof(VertexCount));
	OutFile.write((char *)&IndexCount, sizeof(IndexCount));

	// Write data
	OutFile.write((char *)PackedVertices.data(), sizeof(_PackedVertex) * VertexCount);
	OutFile.write((char *)PackedIndices.data(), sizeof(GLuint) * IndexCount);

	OutFile.close();
}
