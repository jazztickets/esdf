/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/ui/button.h>
#include <ae/ui/label.h>
#include <ae/ui/style.h>
#include <ae/assets.h>
#include <ae/texture.h>
#include <ae/atlas.h>
#include <ae/graphics.h>

// Constructor
_Button::_Button() :
	HoverStyle(nullptr),
	Enabled(false),
	TextureIndex(0) {

}

// Destructor
_Button::~_Button() {
}

// Render the element
void _Button::Render() const {

	if(Style) {

		if(Style->Texture) {
			Graphics.SetProgram(Style->Program);
			Graphics.SetVBO(VBO_NONE);
			Graphics.SetColor(Style->TextureColor);
			Graphics.DrawImage(Bounds, Style->Texture, Style->Stretch);
		}
		else if(Style->Atlas) {
			Graphics.SetProgram(Style->Program);
			Graphics.SetVBO(VBO_NONE);
			Graphics.SetColor(Style->TextureColor);
			Graphics.DrawAtlas(Bounds, Style->Atlas->Texture, Style->Atlas->GetTextureCoords(TextureIndex));
		}
		else {
			Graphics.SetProgram(Assets.Programs["ortho_pos"]);
			Graphics.SetVBO(VBO_NONE);
			Graphics.SetColor(Style->BackgroundColor);
			Graphics.DrawRectangle(Bounds, true);
			Graphics.SetColor(Style->BorderColor);
			Graphics.DrawRectangle(Bounds, false);
		}
	}

	// Draw hover texture
	if(HoverStyle && (Enabled || HitElement)) {

		if(HoverStyle->Texture) {
			Graphics.SetProgram(HoverStyle->Program);
			Graphics.SetVBO(VBO_NONE);
			Graphics.SetColor(HoverStyle->TextureColor);
			Graphics.DrawImage(Bounds, HoverStyle->Texture, Style->Stretch);
		}
		else {
			Graphics.SetProgram(Assets.Programs["ortho_pos"]);
			Graphics.SetVBO(VBO_NONE);
			if(HoverStyle->HasBackgroundColor) {
				Graphics.SetColor(HoverStyle->BackgroundColor);
				Graphics.DrawRectangle(Bounds, true);
			}

			if(HoverStyle->HasBorderColor) {
				Graphics.SetColor(HoverStyle->BorderColor);
				Graphics.DrawRectangle(Bounds, false);
			}
		}
	}

	// Render all children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->Render();
	}
}

// Handle pressed event
void _Button::HandleInput(bool Pressed) {
	if(HitElement) {
	}
}
