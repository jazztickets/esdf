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
#include <objects/prop.h>
#include <graphics.h>
#include <program.h>
#include <mesh.h>
#include <texture.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Constructor
_Prop::_Prop(const _PropStat &Stats) :
	Stats(Stats),
	Program(nullptr),
	Mesh(nullptr) {
}

// Destructor
_Prop::~_Prop() {
}

// Render the mesh
void _Prop::Render() const {
	Graphics.SetProgram(Program);
	glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), Position)));
	Graphics.SetTextureID(Texture->ID);

	glBindBuffer(GL_ARRAY_BUFFER, Mesh->VertexBufferID);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(_PackedVertex), _PackedVertex::GetPositionOffset());
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(_PackedVertex), _PackedVertex::GetUVOffset());
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(_PackedVertex), _PackedVertex::GetNormalOffset());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->ElementBufferID);
	glDrawElements(GL_TRIANGLES, Mesh->IndexCount, GL_UNSIGNED_INT, 0);
}
