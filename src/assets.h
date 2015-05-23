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

// Classes
class _Assets {

	public:

		void Init(bool IsServer);
		void Close();

		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<std::string, const _Font *> Fonts;
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

		void LoadStrings(const std::string &Path);
		void LoadColors(const std::string &Path);
		void LoadTextureDirectory(const std::string &Path, bool IsServer, bool Repeat=false, bool MipMaps=false);
		void LoadMeshDirectory(const std::string &Path);
		void LoadAnimations(const std::string &Path, bool IsServer);
		void LoadFonts(const std::string &Path);
		void LoadPrograms(const std::string &Path);
		void LoadStyles(const std::string &Path);
		void LoadElements(const std::string &Path);
		void LoadLabels(const std::string &Path);
		void LoadImages(const std::string &Path);
		void LoadButtons(const std::string &Path);
		void LoadTextBoxes(const std::string &Path);

		void ResolveElementParents();

		std::unordered_map<std::string, const _Shader *> Shaders;
		std::unordered_map<std::string, _Element *> AllElements;
};

extern _Assets Assets;
