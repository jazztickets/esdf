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
#include <glm/vec2.hpp>
#include <string>
#include <map>
#include <vector>

// Forward Declarations
class _Style;
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
class _Particle;
struct _AnimationTemplate;

// Classes
class _Assets {

	public:

		void Init(bool IsServer);
		void Close();

		void LoadStrings(const std::string &Path);
		void LoadColors(const std::string &Path);
		void LoadTextureDirectory(const std::string &Path, bool Repeat=false, bool MipMaps=false);
		void LoadMeshDirectory(const std::string &Path);
		void LoadAnimations(const std::string &Path, bool IsServer);
		void LoadSamples(const std::string &Path, const std::string &SamplePath);
		void LoadFonts(const std::string &Path);
		void LoadPrograms(const std::string &Path);
		void LoadStyles(const std::string &Path);
		void LoadElements(const std::string &Path);
		void LoadLabels(const std::string &Path);
		void LoadImages(const std::string &Path);
		void LoadButtons(const std::string &Path);
		void LoadTextBoxes(const std::string &Path);

		_Element *GetElement(const std::string &Identifier);
		_Label *GetLabel(const std::string &Identifier);
		_Image *GetImage(const std::string &Identifier);
		_Button *GetButton(const std::string &Identifier);
		_TextBox *GetTextBox(const std::string &Identifier);

		std::map<std::string, std::string> Strings;
		std::map<std::string, const _Font *> Fonts;
		std::map<std::string, const _Texture *> Textures;
		std::map<std::string, const _Mesh *> Meshes;
		std::map<std::string, _Program *> Programs;
		std::map<std::string, const _AnimationTemplate *> AnimationTemplates;
		std::map<std::string, glm::vec4> Colors;
		std::map<std::string, _Style *> Styles;

	private:

		std::map<std::string, const _Shader *> Shaders;
		std::map<std::string, _Element *> Elements;
};

extern _Assets Assets;
