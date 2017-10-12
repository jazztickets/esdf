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
#pragma once

// Libraries
#include <glm/vec4.hpp>
#include <unordered_map>
#include <string>

// Forward Declarations
struct _Style;
class _Font;
class _Element;
class _Label;
class _Image;
class _Button;
class _TextBox;
class _Texture;
class _Mesh;
class _Program;
class _Shader;
struct _AnimationTemplate;

struct _Layer {
	_Layer() : Layer(0), DepthTest(0), DepthMask(0), EditorOnly(0) { }
	int Layer;
	int DepthTest;
	int DepthMask;
	int EditorOnly;
};

// Classes
class _Assets {

	public:

		void Init(bool IsServer);
		void Close();

		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, const _Font *> Fonts;
		std::unordered_map<std::string, _Layer> Layers;
		std::unordered_map<std::string, const _Texture *> Textures;
		std::unordered_map<std::string, const _Mesh *> Meshes;
		std::unordered_map<std::string, _Program *> Programs;
		std::unordered_map<std::string, const _AnimationTemplate *> AnimationTemplates;
		std::unordered_map<std::string, glm::vec4> Colors;
		std::unordered_map<std::string, _Style *> Styles;

		std::unordered_map<std::string, _Element *> Elements;
		std::unordered_map<std::string, _Label *> Labels;
		std::unordered_map<std::string, _Image *> Images;
		std::unordered_map<std::string, _Button *> Buttons;
		std::unordered_map<std::string, _TextBox *> TextBoxes;

	private:

		void LoadColors(const std::string &Path);
		void LoadTextureDirectory(const std::string &Path, bool IsServer, bool Repeat=false, bool MipMaps=false);
		void LoadMeshDirectory(const std::string &Path);
		void LoadAnimations(const std::string &Path, bool IsServer);
		void LoadFonts(const std::string &Path);
		void LoadLayers(const std::string &Path);
		void LoadPrograms(const std::string &Path);
		void LoadStyles(const std::string &Path);
		void LoadElements(const std::string &Path);
		void LoadLabels(const std::string &Path);
		void LoadButtons(const std::string &Path);
		void LoadTextBoxes(const std::string &Path);

		void ResolveElementParents();

		std::unordered_map<std::string, const _Shader *> Shaders;
		std::unordered_map<std::string, _Element *> AllElements;
};

extern _Assets Assets;
