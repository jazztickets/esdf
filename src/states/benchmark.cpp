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
#include <states/benchmark.h>
#include <ae/graphics.h>
#include <framework.h>
#include <ae/assets.h>
#include <ae/camera.h>
#include <ae/texture.h>
#include <ae/mesh.h>
#include <ae/font.h>
#include <ae/program.h>
#include <ae/light.h>
#include <constants.h>
#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_scancode.h>

_BenchmarkState BenchmarkState;
static _Camera *Camera;
static const _Font *Font;
static const _Texture *Texture;

void _BenchmarkState::Init() {
	SDL_GL_SetSwapInterval(1);

	Camera = new _Camera(glm::vec3(-2, -2, 7), 200, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->Set2DPosition(glm::vec2(2, 2));
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Font = Assets.Fonts["hud_tiny"];

	//_Mesh::ConvertOBJ("meshes/tree.obj");
}

void _BenchmarkState::Close() {

	delete Texture;
	delete Camera;
}

// Key handler
void _BenchmarkState::HandleKey(const _KeyEvent &KeyEvent) {
	if(KeyEvent.Pressed) {
		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_ESCAPE:
				Framework.SetDone(true);
			break;
		}
	}
}

// Update
void _BenchmarkState::Update(double FrameTime) {
	Camera->Update(FrameTime);
}

// Render the state
void _BenchmarkState::Render(double BlendFactor) {
	Assets.Programs["pos_uv_norm"]->AmbientLight = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	Assets.Programs["pos_uv_norm"]->LightCount = 1;
	Assets.Programs["pos_uv_norm"]->Lights[0].Color = glm::vec4(1, 1, 1, 1);
	Assets.Programs["pos_uv_norm"]->Lights[0].Position = glm::vec3(-2, -2, 3.0f);

	Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	Graphics.DirtyState();
	Graphics.Setup2D();

	// FPS
	if(1) {
		Graphics.SetVBO(VBO_NONE);
		std::ostringstream Buffer;
		Buffer << Graphics.FramesPerSecond;
		Font->DrawText(Buffer.str(), glm::vec2(5, 5), LEFT_TOP, glm::vec4(1, 1, 1, 1));
		Buffer.str("");
	}
}
