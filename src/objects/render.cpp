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
#include <graphics.h>
#include <texture.h>

// Constructor
_Render::_Render(_Object *Parent) :
	Parent(Parent),
	Icon(nullptr),
	Color(1.0f),
	Scale(1.0f),
	Z(0.0f),
	Layer(0) {

}

// Destructor
_Render::~_Render() {
}

// Draw the object
void _Render::Draw3D(double BlendFactor) {

	/*
	if(Parent->Physics->Interpolate) {
		for(int i = 0; i < Parent->Physics->History.Size(); i++) {
			const glm::vec2 &P = Parent->Physics->History.Back(i).Position;
			Graphics.DrawTexture(P.X, P.Y, Parent->PositionZ, Icon, COLOR_BLUE, Parent->Physics->InterpolatedRotation, Parent->Scale, Parent->Scale);
		}
	}
	*/

	glm::vec2 DrawPosition = Parent->Physics->Position * (float)BlendFactor + Parent->Physics->LastPosition * (1.0f - (float)BlendFactor);

	// TODO - should just be using rotation
	float DrawRotation;
	if(Parent->Physics->Interpolate)
		DrawRotation = Parent->Physics->InterpolatedRotation;
	else
		DrawRotation = Parent->Physics->Rotation;

	if(Parent->Animation) {
		Graphics.SetVBO(VBO_ATLAS);
		Graphics.UpdateVBOTextureCoords(VBO_ATLAS, Parent->Animation->TextureCoords);

		// Draw server position
		if(0) {
				Graphics.DrawSprite(
				glm::vec3(Parent->Physics->NetworkPosition, Z),
				Parent->Animation->Templates[Parent->Animation->Reel]->Texture,
				glm::vec4(1.0f, 0, 0, 1.0f),
				DrawRotation,
				glm::vec2(Scale)
			);
		}

		// Draw animation frame
		Graphics.DrawSprite(
			glm::vec3(DrawPosition, Z),
			Parent->Animation->Templates[Parent->Animation->Reel]->Texture,
			glm::vec4(1.0f),
			DrawRotation,
			glm::vec2(Scale)
		);
	}
	else {
		Graphics.SetVBO(VBO_QUAD);
		Graphics.DrawSprite(
			glm::vec3(DrawPosition, Z),
			Icon,
			glm::vec4(1.0f),
			DrawRotation,
			glm::vec2(Scale)
		);
	}
}