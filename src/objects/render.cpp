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
#include <objects/render.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/animation.h>
#include <objects/shape.h>
#include <objects/shot.h>
#include <stats.h>
#include <ae/graphics.h>
#include <ae/buffer.h>
#include <ae/program.h>
#include <ae/mesh.h>
#include <ae/texture.h>
#include <stats.h>
#include <ae/assets.h>
#include <constants.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ae/font.h>
#include <sstream>

// Constructor
_Render::_Render(_Object *Parent, const _RenderStat *Stats) :
	_Component(Parent),
	Stats(Stats),
	Texture(nullptr),
	Color(1.0f),
	Debug(0) {

}

// Destructor
_Render::~_Render() {
}

// Serialize
void _Render::NetworkSerialize(ae::_Buffer &Buffer) {
	if(!Texture)
		Buffer.WriteString("");
	else
		Buffer.WriteString(Texture->Name.c_str());
}

// Unserialize
void _Render::NetworkUnserialize(ae::_Buffer &Buffer) {
	std::string TextureIdentifier = Buffer.ReadString();
	Texture = ae::Assets.Textures[TextureIdentifier];
}

// Draw the object
void _Render::Draw3D(double BlendFactor) {
	ae::Graphics.SetProgram(Program);

	/*
	if(Parent->Physics->Interpolate) {
		for(int i = 0; i < Parent->Physics->History.Size(); i++) {
			const glm::vec2 &P = Parent->Physics->History.Back(i).Position;
			ae::Graphics.DrawTexture(P.X, P.Y, Parent->PositionZ, Icon, COLOR_BLUE, Parent->Physics->InterpolatedRotation, Parent->Scale, Parent->Scale);
		}
	}
	*/

	glm::vec3 DrawPosition;
	float DrawRotation = 0.0f;

	if(Parent->Physics) {
		DrawPosition = Parent->Physics->Position * (float)BlendFactor + Parent->Physics->LastPosition * (1.0f - (float)BlendFactor);

		// TODO - should just be using rotation
		if(Parent->Physics->RenderDelay)
			DrawRotation = Parent->Physics->InterpolatedRotation;
		else
			DrawRotation = Parent->Physics->Rotation;
	}

	ae::Graphics.SetColor(Color);
	if(Parent->Animation) {

		// Draw server position
		if(Debug & DEBUG_NETWORK) {
			ae::Graphics.SetColor(glm::vec4(1.0f, 0, 0, 1.0f));
			ae::Graphics.DrawAnimationFrame(
				glm::vec3(Parent->Physics->NetworkPosition.x, Parent->Physics->NetworkPosition.y, Stats->Z),
				Parent->Animation->Templates[Parent->Animation->Reel]->Texture,
				Parent->Animation->TextureCoords,
				DrawRotation,
				glm::vec2(Stats->Scale)
			);
			ae::Graphics.SetColor(Color);
		}

		// Draw animation frame
		ae::Graphics.DrawAnimationFrame(
			glm::vec3(DrawPosition.x, DrawPosition.y, Stats->Z),
			Parent->Animation->Templates[Parent->Animation->Reel]->Texture,
			glm::vec4(Parent->Animation->TextureCoords),
			DrawRotation,
			glm::vec2(Stats->Scale)
		);
	}
	else if(Mesh) {
		glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(DrawPosition.x, DrawPosition.y, Stats->Z))));
		ae::Graphics.SetTextureID(Texture->ID);
		ae::Graphics.SetVertexBufferID(Mesh->VertexBufferID);
		ae::Graphics.EnableAttribs(3);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ae::_PackedVertex), ae::_PackedVertex::GetPositionOffset());
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ae::_PackedVertex), ae::_PackedVertex::GetUVOffset());
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ae::_PackedVertex), ae::_PackedVertex::GetNormalOffset());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->ElementBufferID);
		glDrawElements(GL_TRIANGLES, Mesh->IndexCount, GL_UNSIGNED_INT, 0);
	}
	// Draw cube
	else if(Stats->Layer == 0) {
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawCube(DrawPosition - Parent->Shape->HalfWidth, Parent->Shape->HalfWidth * 2.0f, Texture);
	}
	else if(Texture) {

		// Draw server position
		if(Debug & DEBUG_NETWORK) {
			ae::Graphics.SetColor(glm::vec4(1.0f, 0, 0, 1.0f));
			ae::Graphics.DrawSprite(
				glm::vec3(Parent->Physics->NetworkPosition.x, Parent->Physics->NetworkPosition.y, Stats->Z),
				Texture,
				DrawRotation,
				glm::vec2(Stats->Scale)
			);
			ae::Graphics.SetColor(Color);
		}

		ae::Graphics.DrawSprite(
			glm::vec3(DrawPosition.x, DrawPosition.y, Stats->Z),
			Texture,
			DrawRotation,
			glm::vec2(Stats->Scale)
		);
	}
	else {
		ae::Graphics.DrawRectangle3D(glm::vec2(DrawPosition - Parent->Shape->HalfWidth), glm::vec2(DrawPosition + Parent->Shape->HalfWidth), true);
	}

	// Draw object id
	if(Debug & DEBUG_ID) {
		ae::Graphics.SetDepthTest(false);
		std::ostringstream Buffer;
		Buffer << Parent->NetworkID;
		ae::Assets.Fonts["menu_buttons"]->DrawText(Buffer.str(), glm::vec2(Parent->Physics->Position), ae::CENTER_BASELINE, glm::vec4(1.0f), 1.0f / 64.0f);
		ae::Graphics.SetDepthTest(true);
	}
}
