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
#include <objects/render.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/animation.h>
#include <objects/shape.h>
#include <objects/shot.h>
#include <stats.h>
#include <graphics.h>
#include <buffer.h>
#include <program.h>
#include <mesh.h>
#include <texture.h>
#include <stats.h>
#include <assets.h>
#include <constants.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Constructor
_Render::_Render(_Object *Parent, const _RenderStat &Stat) :
	_Component(Parent),
	Stats(Stat),
	Texture(nullptr),
	Color(1.0f) {

}

// Destructor
_Render::~_Render() {
}

// Serialize
void _Render::NetworkSerialize(_Buffer &Buffer) {
	if(!Texture)
		Buffer.WriteString("");
	else
		Buffer.WriteString(Texture->Identifier.c_str());
}

// Unserialize
void _Render::NetworkUnserialize(_Buffer &Buffer) {
	std::string TextureIdentifier = Buffer.ReadString();
	Texture = Assets.Textures[TextureIdentifier];
}

// Draw the object
void _Render::Draw3D(double BlendFactor) {
	Graphics.SetProgram(Program);

	/*
	if(Parent->Physics->Interpolate) {
		for(int i = 0; i < Parent->Physics->History.Size(); i++) {
			const glm::vec2 &P = Parent->Physics->History.Back(i).Position;
			Graphics.DrawTexture(P.X, P.Y, Parent->PositionZ, Icon, COLOR_BLUE, Parent->Physics->InterpolatedRotation, Parent->Scale, Parent->Scale);
		}
	}
	*/

	glm::vec3 DrawPosition;
	float DrawRotation = 0.0f;

	if(Parent->Physics) {
		DrawPosition = Parent->Physics->Position * (float)BlendFactor + Parent->Physics->LastPosition * (1.0f - (float)BlendFactor);

		// TODO - should just be using rotation
		if(Parent->Physics->Interpolate)
			DrawRotation = Parent->Physics->InterpolatedRotation;
		else
			DrawRotation = Parent->Physics->Rotation;
	}

	Graphics.SetColor(Color);
	if(Parent->Animation) {
		Graphics.SetVBO(VBO_ATLAS);
		Graphics.UpdateVBOTextureCoords(VBO_ATLAS, Parent->Animation->TextureCoords);

		// Draw server position
		if(0) {
			Graphics.SetColor(glm::vec4(1.0f, 0, 0, 1.0f));
			Graphics.DrawSprite(
				glm::vec3(Parent->Physics->NetworkPosition.x, Parent->Physics->NetworkPosition.y, Stats.Z),
				Parent->Animation->Templates[Parent->Animation->Reel]->Texture,
				DrawRotation,
				glm::vec2(Stats.Scale)
			);
		}

		// Draw animation frame
		Graphics.DrawSprite(
			glm::vec3(DrawPosition.x, DrawPosition.y, Stats.Z),
			Parent->Animation->Templates[Parent->Animation->Reel]->Texture,
			DrawRotation,
			glm::vec2(Stats.Scale)
		);
	}
	else if(Mesh) {
		glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(DrawPosition.x, DrawPosition.y, Stats.Z))));
		Graphics.SetTextureID(Texture->ID);
		Graphics.SetVertexBufferID(Mesh->VertexBufferID);
		Graphics.EnableAttribs(3);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(_PackedVertex), _PackedVertex::GetPositionOffset());
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(_PackedVertex), _PackedVertex::GetUVOffset());
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(_PackedVertex), _PackedVertex::GetNormalOffset());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->ElementBufferID);
		glDrawElements(GL_TRIANGLES, Mesh->IndexCount, GL_UNSIGNED_INT, 0);
	}
	// Draw cube
	else if(Stats.Layer == 0) {
		Graphics.SetVBO(VBO_CUBE);
		Graphics.SetColor(glm::vec4(1.0f));
		Graphics.DrawCube(DrawPosition - Parent->Shape->HalfWidth, Parent->Shape->HalfWidth * 2.0f, Texture);
	}
	else if(Texture) {
		Graphics.SetVBO(VBO_QUAD);
		Graphics.DrawSprite(
			glm::vec3(DrawPosition.x, DrawPosition.y, Stats.Z),
			Texture,
			DrawRotation,
			glm::vec2(Stats.Scale)
		);
	}
	else {
		Graphics.SetVBO(VBO_NONE);
		Graphics.DrawRectangle(glm::vec2(DrawPosition - Parent->Shape->HalfWidth), glm::vec2(DrawPosition + Parent->Shape->HalfWidth), true);
	}
}
