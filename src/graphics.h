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

// Libraries
#include <opengl.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <SDL_video.h>

// Forward Declarations
class _Texture;
class _Program;
class _Element;
class _Bounds;
class _Log;

enum VertexBufferType {
	VBO_NONE,
	VBO_CIRCLE,
	VBO_QUAD,
	VBO_ATLAS,
	VBO_CUBE,
	VBO_COUNT
};

// Classes
class _Graphics {

	public:

		_Graphics() : Enabled(false), LastColor(1.0f) { }

		void Init(const glm::ivec2 &WindowSize, const glm::ivec2 &WindowPosition, int Vsync, int MSAA, int Anisotropy, bool Fullscreen, _Log *Log);
		void Close();

		void ToggleFullScreen();
		void ShowCursor(bool Show);
		void BuildVertexBuffers();

		void SetStaticUniforms();
		void ChangeViewport(const glm::ivec2 &Size);
		void Setup2D();
		void Setup3D();

		void FadeScreen(float Amount);
		void DrawImage(const _Bounds &Bounds, const _Texture *Texture, const glm::vec4 &Color, bool Stretch=false);
		void DrawAtlas(const _Bounds &Bounds, const _Texture *Texture, const glm::vec4 &TextureCoords, const glm::vec4 &Color);
		void DrawRectangle(const _Bounds &Bounds, const glm::vec4 &Color, bool Filled=false);
		void DrawMask(const _Bounds &Bounds);

		void DrawSprite(const glm::vec3 &Position, const _Texture *Texture, const glm::vec4 &Color, float Rotation=0.0f, const glm::vec2 Scale=glm::vec2(1.0f));
		void DrawTile(const glm::vec2 &Start, const glm::vec2 &End, float Z, const _Texture *Texture);
		void DrawCube(const glm::vec3 &Start, const glm::vec3 &Scale, const _Texture *Texture);
		void DrawRectangle(const glm::vec2 &Start, const glm::vec2 &End, const glm::vec4 &Color, bool Filled=false);
		void DrawCircle(const glm::vec3 &Position, float Radius, const glm::vec4 &Color);

		void SetDepthMask(bool Value);
		void EnableStencilTest();
		void DisableStencilTest();
		void EnableParticleBlending();
		void DisableParticleBlending();
		void ClearScreen();
		void Flip(double FrameTime);

		GLuint CreateVBO(float *Triangles, GLuint Size, GLenum Type);
		void UpdateVBOTextureCoords(int VBO, float *Data);
		void SetVBO(GLuint Type);
		void EnableAttribs(int AttribLevel);

		void SetColor(const glm::vec4 &Color);
		void SetTextureID(GLuint TextureID);
		void SetProgram(const _Program *Program);
		void SetDepthTest(bool DepthTest);

		void DirtyState();

		// State
		_Element *Element;
		int FramesPerSecond;
		glm::ivec2 WindowSize;
		glm::ivec2 ViewportSize;
		float AspectRatio;
		GLfloat Anisotropy;
		GLuint VertexBuffer[VBO_COUNT];
		glm::mat4 Ortho;

	private:

		void SetupOpenGL();

		// Data structures
		bool Enabled;
		SDL_Window *Window;
		SDL_GLContext Context;

		// State changes
		GLuint LastVBO;
		GLuint LastTextureID;
		int LastAttribLevel;
		glm::vec4 LastColor;
		const _Program *LastProgram;
		bool LastDepthTest;

		// Benchmarking
		double FrameRateTimer;
		int FrameCount;
};

extern _Graphics Graphics;
