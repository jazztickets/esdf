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

#include <opengl.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>
#include <cstring>

// Struct for an interleaved vertex
struct _PackedVertex {

	bool operator<(const _PackedVertex &PackedVertex) const {
		return memcmp((void *)this, (void *)&PackedVertex, sizeof(_PackedVertex)) > 0;
	}

	static const void *GetPositionOffset() { return (void *)&(((_PackedVertex *)0)->Position); }
	static const void *GetUVOffset() { return (void *)&(((_PackedVertex *)0)->UV); }
	static const void *GetNormalOffset() { return (void *)&(((_PackedVertex *)0)->Normal); }

	glm::vec3 Position;
	glm::vec2 UV;
	glm::vec3 Normal;
};

// Triangle Mesh
class _Mesh {

	public:

		enum _Flags {
			HAS_UVS     = (1 << 0),
			HAS_NORMALS = (1 << 1),
		};

		_Mesh(const std::string &Path);
		~_Mesh();

		static void ConvertOBJ(const std::string &Path);

		// Attributes
		std::string Identifier;
		uint32_t IndexCount;
		uint32_t Flags;
		uint8_t Version;

		// VBO
		GLuint VertexBufferID;
		GLuint ElementBufferID;

	private:

};
