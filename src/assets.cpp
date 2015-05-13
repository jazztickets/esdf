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
#include <assets.h>
#include <objects/animation.h>
#include <objects/particle.h>
#include <ui/style.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/button.h>
#include <ui/image.h>
#include <ui/textbox.h>
#include <program.h>
#include <font.h>
#include <texture.h>
#include <mesh.h>
#include <audio.h>
#include <utils.h>
#include <files.h>
#include <graphics.h>
#include <constants.h>
#include <stdexcept>
#include <iostream>

_Assets Assets;

// Initialize
void _Assets::Init(bool IsServer) {
	LoadStrings(ASSETS_STRINGS);
	if(!IsServer) {
		LoadPrograms(ASSETS_PROGRAMS);
		LoadFonts(ASSETS_FONT_TABLE);
		LoadTextureDirectory(TEXTURES_HUD);
		LoadTextureDirectory(TEXTURES_HUD_REPEAT, true);
		LoadTextureDirectory(TEXTURES_EDITOR);
		LoadTextureDirectory(TEXTURES_MENU);
		LoadTextureDirectory(TEXTURES_TILES);
		LoadTextureDirectory(TEXTURES_BLOCKS, true, true);
		LoadTextureDirectory(TEXTURES_PROPS, true, true);
		LoadMeshDirectory(MESHES_PATH);
		LoadColors(ASSETS_COLORS);

		LoadStyles(ASSETS_UI_STYLES);
		LoadElements(ASSETS_UI_ELEMENTS);
		LoadImages(ASSETS_UI_IMAGES);
		LoadButtons(ASSETS_UI_BUTTONS);
		LoadTextBoxes(ASSETS_UI_TEXTBOXES);
		LoadLabels(ASSETS_UI_LABELS);
	}

	LoadAnimations(ASSETS_ANIMATIONS, IsServer);
}

// Shutdown
void _Assets::Close() {

	for(const auto &Program : Programs)
		delete Program.second;

	for(const auto &Shader : Shaders)
		delete Shader.second;

	for(const auto &Texture : Textures)
		delete Texture.second;

	for(const auto &Mesh : Meshes)
		delete Mesh.second;

	for(const auto &Font : Fonts)
		delete Font.second;

	for(const auto &Style : Styles)
		delete Style.second;

	for(const auto &Label : Labels)
		delete Label.second;

	for(const auto &Element : Elements)
		delete Element.second;

	for(const auto &AnimationTemplate : AnimationTemplates)
		delete AnimationTemplate.second;

	Fonts.clear();
	Textures.clear();
	Meshes.clear();
	Styles.clear();
	Labels.clear();
	Elements.clear();
	AnimationTemplates.clear();
}

// Loads the strings
void _Assets::LoadStrings(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Ignore the first line
	File.ignore(1024, '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {

		// Load data
		std::string Identifier = GetTSVText(File);
		std::string Text = GetTSVText(File);

		// Check for duplicates
		if(Strings[Identifier] != "")
			throw std::runtime_error("LoadStringTable - Duplicate entry: " + Identifier);

		Strings[Identifier] = Text;
	}

	File.close();
}

// Loads the fonts
void _Assets::LoadFonts(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Ignore the first line
	File.ignore(1024, '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier = GetTSVText(File);
		std::string FontFile = GetTSVText(File);
		std::string ProgramIdentifier = GetTSVText(File);

		// Check for duplicates
		if(Fonts[Identifier])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		// Find program
		if(Programs.find(ProgramIdentifier) == Programs.end())
		   throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find program: " + ProgramIdentifier);

		// Get size
		int Size;
		File >> Size;

		File.ignore(1024, '\n');

		// Load font
		Fonts[Identifier] = new _Font(ASSETS_FONTS + FontFile, Programs[ProgramIdentifier], Size);
	}

	File.close();
}

// Load shader programs
void _Assets::LoadPrograms(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Ignore the first line
	File.ignore(1024, '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier = GetTSVText(File);
		std::string VertexPath = GetTSVText(File);
		std::string FragmentPath = GetTSVText(File);

		// Get attrib count
		int Attribs;
		File >> Attribs;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Programs[Identifier])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		// Load vertex shader
		if(Shaders.find(VertexPath) == Shaders.end())
			Shaders[VertexPath] = new _Shader(VertexPath, GL_VERTEX_SHADER);

		// Load fragment shader
		if(Shaders.find(FragmentPath) == Shaders.end())
			Shaders[FragmentPath] = new _Shader(FragmentPath, GL_FRAGMENT_SHADER);

		// Create program
		Programs[Identifier]= new _Program(Shaders[VertexPath], Shaders[FragmentPath], Attribs);
	}

	File.close();
}

// Loads the color table
void _Assets::LoadColors(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(1024, '\n');

	// Add default color
	glm::vec4 Color(1.0f);
	Colors[""] = Color;

	// Read table
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		File >> Color.r >> Color.g >> Color.b >> Color.a;
		File.ignore(1024, '\n');

		// Check for duplicates
		if(Colors.find(Identifier) != Colors.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		Colors[Identifier] = Color;
	}

	File.close();
}

// Load a directory full of textures
void _Assets::LoadTextureDirectory(const std::string &Path, bool Repeat, bool MipMaps) {

	// Get files
	_Files Files(TEXTURES_PATH + Path);

	// Load textures
	for(const auto &File : Files.Nodes) {
		std::string Identifier = Path + File;
		if(!Assets.Textures[Identifier])
			Assets.Textures[Identifier] = new _Texture(Identifier, Repeat, MipMaps);
	}
}

// Load meshes
void _Assets::LoadMeshDirectory(const std::string &Path) {

	// Get files
	_Files Files(Path);

	// Load meshes
	for(const auto &File : Files.Nodes) {
		if(File.find(MESHES_SUFFIX) != std::string::npos) {
			std::string Identifier = Path + File;
			Assets.Meshes[Identifier] = new _Mesh(Identifier);
		}
	}
}

// Load animations
void _Assets::LoadAnimations(const std::string &Path, bool IsServer) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier = GetTSVText(File);

		// Check for duplicates
		if(AnimationTemplates[Identifier])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		// Create template
		_AnimationTemplate *Template = new _AnimationTemplate();
		Template->Identifier = Identifier;

		// Load texture
		std::string TextureFile = GetTSVText(File);
		if(!IsServer) {
			if(!Assets.Textures[TextureFile])
				Assets.Textures[TextureFile] = new _Texture(TextureFile, false, false);

			Template->Texture = Assets.Textures[TextureFile];
		}
		else
			Template->Texture = nullptr;

		// Read data
		File >> Template->FrameSize.x >> Template->FrameSize.y >> Template->StartFrame >> Template->EndFrame >> Template->DefaultFrame >> Template->RepeatType;
		File.ignore(1024, '\n');

		// Add to list
		if(!IsServer) {
			Template->FramesPerLine = Template->Texture->Size.x / Template->FrameSize.x;
			Template->TextureScale = glm::vec2(Template->FrameSize) / glm::vec2(Template->Texture->Size);
		}
		AnimationTemplates[Identifier] = Template;
	}

	File.close();
}

// Loads the styles
void _Assets::LoadStyles(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(1024, '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string BackgroundColorIdentifier = GetTSVText(File);
		std::string BorderColorIdentifier = GetTSVText(File);
		std::string ProgramIdentifier = GetTSVText(File);
		std::string TextureIdentifier = GetTSVText(File);
		std::string TextureColorIdentifier = GetTSVText(File);

		// Find program
		if(Programs.find(ProgramIdentifier) == Programs.end())
		   throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find program: " + ProgramIdentifier);

		bool Stretch;
		File >> Stretch;
		File.ignore(1024, '\n');

		// Get colors
		glm::vec4 BackgroundColor = Colors[BackgroundColorIdentifier];
		glm::vec4 BorderColor = Colors[BorderColorIdentifier];
		glm::vec4 TextureColor = Colors[TextureColorIdentifier];

		// Get textures
		const _Texture *Texture = Textures[TextureIdentifier];

		// Create style
		_Style *Style = new _Style;
		Style->Identifier = Identifier;
		Style->HasBackgroundColor = BackgroundColorIdentifier != "";
		Style->HasBorderColor = BorderColorIdentifier != "";
		Style->BackgroundColor = BackgroundColor;
		Style->BorderColor = BorderColor;
		Style->Program = Programs[ProgramIdentifier];
		Style->Texture = Texture;
		Style->Atlas = nullptr;
		Style->TextureColor = TextureColor;
		Style->Stretch = Stretch;

		// Check for duplicates
		if(Styles.find(Identifier) != Styles.end())
			throw std::runtime_error("Duplicate style identifier: " + Identifier);

		Styles[Identifier] = Style;
	}

	File.close();
}

// Loads the ui elements
void _Assets::LoadElements(const std::string &Filename) {

	// Load file
	std::ifstream File(Filename.c_str(), std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Filename);
	}

	// Read the file
	File.ignore(1024, '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		bool MaskOutside;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaskOutside;
		File.ignore(1024, '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = GetElement(ParentIdentifier);
			if(!ParentElement) {
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
			}
		}

		// Get style
		_Style *Style = nullptr;
		if(StyleIdentifier != "") {
			Style = Styles[StyleIdentifier];
			if(!Style)
				throw std::runtime_error("Unable to find style: " + StyleIdentifier);
		}

		// Create
		_Element *Element = new _Element(Identifier, ParentElement, Offset, Size, Alignment, Style, MaskOutside);
		Graphics.Element->AddChild(Element);

		// Check for duplicates
		if(GetElement(Identifier)) {
			throw std::runtime_error("Duplicate element identifier: " + Identifier);
		}

		// Add as child for parent
		if(ParentElement) {
			ParentElement->AddChild(Element);
		}

		Elements.insert(make_pair(Identifier, Element));
	}

	File.close();
}

// Loads labels elements
void _Assets::LoadLabels(const std::string &Filename) {

	// Load file
	std::ifstream File(Filename.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Filename);

	// Read the file
	File.ignore(1024, '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string FontIdentifier = GetTSVText(File);
		std::string ColorIdentifier = GetTSVText(File);
		std::string Text = GetTSVText(File);

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical;
		File.ignore(1024, '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = GetElement(ParentIdentifier);
			if(!ParentElement)
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
		}

		// Get font
		const _Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Get color
		glm::vec4 Color = Colors[ColorIdentifier];

		// Create
		_Label *Element = new _Label(Identifier, ParentElement, Offset, Size, Alignment, Font, Color, Text);

		// Check for duplicates
		if(Labels.find(Identifier) != Labels.end())
			throw std::runtime_error("Duplicate label identifier: " + Identifier);

		// Add as child for parent
		if(ParentElement)
			ParentElement->AddChild(Element);

		Labels[Identifier] = Element;
	}

	File.close();
}

// Loads image elements
void _Assets::LoadImages(const std::string &Filename) {

	// Load file
	std::ifstream File(Filename.c_str(), std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Filename);
	}

	// Read the file
	File.ignore(1024, '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string TextureIdentifier = GetTSVText(File);
		std::string ColorIdentifier = GetTSVText(File);

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		int Stretch;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> Stretch;
		File.ignore(1024, '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = GetElement(ParentIdentifier);
			if(!ParentElement)
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
		}

		// Get texture
		const _Texture *Texture = Textures[TextureIdentifier];

		// Get color
		glm::vec4 Color = Colors[ColorIdentifier];

		// Create
		_Image *Element = new _Image(Identifier, ParentElement, Offset, Size, Alignment, Texture, Color, Stretch);

		// Check for duplicates
		if(GetElement(Identifier))
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Add as child for parent
		if(ParentElement) {
			ParentElement->AddChild(Element);
		}

		Elements.insert(make_pair(Identifier, Element));
	}

	File.close();
}

// Loads button elements
void _Assets::LoadButtons(const std::string &Filename) {

	// Load file
	std::ifstream File(Filename.c_str(), std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Filename);
	}

	// Read the file
	File.ignore(1024, '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);
		std::string HoverStyleIdentifier = GetTSVText(File);

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical;
		File.ignore(1024, '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = GetElement(ParentIdentifier);
			if(!ParentElement) {
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
			}
		}

		// Get style
		_Style *Style = Styles[StyleIdentifier];
		_Style *HoverStyle = Styles[HoverStyleIdentifier];

		// Create
		_Button *Element = new _Button(Identifier, ParentElement, Offset, Size, Alignment, Style, HoverStyle);

		// Check for duplicates
		if(GetElement(Identifier)) {
			throw std::runtime_error("Duplicate element identifier: " + Identifier);
		}

		// Add as child for parent
		if(ParentElement) {
			ParentElement->AddChild(Element);
		}

		Elements.insert(make_pair(Identifier, Element));
	}

	File.close();
}

// Loads textbox elements
void _Assets::LoadTextBoxes(const std::string &Filename) {

	// Load file
	std::ifstream File(Filename.c_str(), std::ios::in);
	if(!File) {
		throw std::runtime_error("Error loading: " + Filename);
	}

	// Read the file
	File.ignore(1024, '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);
		std::string FontIdentifier = GetTSVText(File);

		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		int MaxLength;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaxLength;
		File.ignore(1024, '\n');

		// Look for parent
		_Element *ParentElement = nullptr;
		if(ParentIdentifier != "") {
			ParentElement = GetElement(ParentIdentifier);
			if(!ParentElement) {
				throw std::runtime_error("Parent element not found: " + ParentIdentifier);
			}
		}

		// Get style
		_Style *Style = Styles[StyleIdentifier];

		// Get font
		const _Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Create
		_TextBox *Element = new _TextBox(Identifier, ParentElement, Offset, Size, Alignment, Style, Font, MaxLength);

		// Check for duplicates
		if(GetElement(Identifier)) {
			throw std::runtime_error("Duplicate element identifier: " + Identifier);
		}

		// Add as child for parent
		if(ParentElement) {
			ParentElement->AddChild(Element);
		}

		Elements.insert(make_pair(Identifier, Element));
	}

	File.close();
}

_Element *_Assets::GetElement(const std::string &Identifier) {
	if(Elements.find(Identifier) == Elements.end())
		return nullptr;

	return Elements[Identifier];
}
_Image *_Assets::GetImage(const std::string &Identifier) { return (_Image *)GetElement(Identifier); }
_Button *_Assets::GetButton(const std::string &Identifier) { return (_Button *)GetElement(Identifier); }
_TextBox *_Assets::GetTextBox(const std::string &Identifier) { return (_TextBox *)GetElement(Identifier); }
