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
#include <ae/assets.h>
#include <objects/animation.h>
#include <ae/ui/style.h>
#include <ae/ui/element.h>
#include <ae/ui/label.h>
#include <ae/ui/button.h>
#include <ae/ui/image.h>
#include <ae/ui/textbox.h>
#include <ae/program.h>
#include <ae/font.h>
#include <ae/texture.h>
#include <ae/mesh.h>
#include <ae/files.h>
#include <ae/graphics.h>
#include <ae/audio.h>
#include <ae/util.h>
#include <constants.h>
#include <map>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <limits>
#include <tinyxml2.h>

_Assets Assets;

// Initialize
void _Assets::Init() {
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

	for(const auto &Sound : Sounds)
		delete Sound.second;

	for(const auto &Song : Music)
		delete Song.second;

	for(const auto &Style : Styles)
		delete Style.second;

	for(const auto &Element : AllElements)
		delete Element.second;

	for(const auto &AnimationTemplate : AnimationTemplates)
		delete AnimationTemplate.second;

	Fonts.clear();
	Layers.clear();
	Textures.clear();
	Meshes.clear();
	Styles.clear();
	AnimationTemplates.clear();
	Elements.clear();
	Labels.clear();
	Images.clear();
	Buttons.clear();
	TextBoxes.clear();
	AllElements.clear();
}

// Loads the fonts
void _Assets::LoadFonts(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {

		// Read strings
		std::string Name;
		std::string FontFile;
		std::string ProgramName;
		std::getline(File, Name, '\t');
		std::getline(File, FontFile, '\t');
		std::getline(File, ProgramName, '\t');

		// Check for duplicates
		if(Fonts[Name])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		// Find program
		if(Programs.find(ProgramName) == Programs.end())
		   throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find program: " + ProgramName);

		// Get size
		uint32_t Size;
		File >> Size;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Load font
		Fonts[Name] = new _Font(Name, FontFile, Programs[ProgramName], Size);
	}

	File.close();
}

// Load render layers
void _Assets::LoadLayers(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Name;
		std::getline(File, Name, '\t');

		// Get layer
		_Layer Layer;
		File >> Layer.Layer >> Layer.DepthTest >> Layer.DepthMask >> Layer.EditorOnly;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Set layer
		Layers[Name] = Layer;
	}

	File.close();
}

// Load shader programs
void _Assets::LoadPrograms(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Name;
		std::string VertexPath;
		std::string FragmentPath;
		std::getline(File, Name, '\t');
		std::getline(File, VertexPath, '\t');
		std::getline(File, FragmentPath, '\t');

		// Get attrib count
		int Attribs;
		File >> Attribs;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Programs[Name])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		// Load vertex shader
		if(Shaders.find(VertexPath) == Shaders.end())
			Shaders[VertexPath] = new _Shader(VertexPath, GL_VERTEX_SHADER);

		// Load fragment shader
		if(Shaders.find(FragmentPath) == Shaders.end())
			Shaders[FragmentPath] = new _Shader(FragmentPath, GL_FRAGMENT_SHADER);

		// Create program
		Programs[Name] = new _Program(Name, Shaders[VertexPath], Shaders[FragmentPath], Attribs);
	}

	File.close();
}

// Loads the color table
void _Assets::LoadColors(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Add default color
	glm::vec4 Color(1.0f);
	Colors[""] = Color;

	// Read table
	while(!File.eof() && File.peek() != EOF) {

		std::string Name;
		std::getline(File, Name, '\t');

		File >> Color.r >> Color.g >> Color.b >> Color.a;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Colors.find(Name) != Colors.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		Colors[Name] = Color;
	}

	File.close();
}

// Load a directory full of textures
void _Assets::LoadTextureDirectory(const std::string &Path, bool IsServer, bool Repeat, bool MipMaps) {

	// Get files
	_Files Files(Path);

	// Load textures
	for(const auto &File : Files.Nodes) {
		std::string Name = Path + File;
		if(!Assets.Textures[Name])
			Assets.Textures[Name] = new _Texture(Name, IsServer, Repeat, MipMaps);
	}
}

// Load game audio
void _Assets::LoadSounds(const std::string &Path) {

	// Get files
	_Files Files(Path);

	// Load audio
	for(const auto &File : Files.Nodes) {
		if(!Assets.Sounds[File])
			Assets.Sounds[File] = Audio.LoadSound(Path + File);
	}
}

// Load music files
void _Assets::LoadMusic(const std::string &Path) {

	// Get files
	_Files Files(Path);

	// Load audio
	for(const auto &File : Files.Nodes) {
		if(!Assets.Music[File])
			Assets.Music[File] = Audio.LoadMusic(Path + File);
	}
}

// Load meshes
void _Assets::LoadMeshDirectory(const std::string &Path) {

	// Get files
	_Files Files(Path);

	// Load meshes
	for(const auto &File : Files.Nodes) {
		if(File.find(".mesh") != std::string::npos) {
			std::string Name = Path + File;
			Assets.Meshes[Name] = new _Mesh(Name);
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
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {
		std::string Name;
		std::getline(File, Name, '\t');

		// Check for duplicates
		if(AnimationTemplates[Name])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		// Create template
		_AnimationTemplate *Template = new _AnimationTemplate();
		Template->Identifier = Name;

		// Load texture
		std::string TextureFile;
		std::getline(File, TextureFile, '\t');
		if(!Assets.Textures[TextureFile])
			Assets.Textures[TextureFile] = new _Texture(TextureFile, IsServer, false, false);

		Template->Texture = Assets.Textures[TextureFile];

		// Read data
		File >> Template->FrameSize.x >> Template->FrameSize.y >> Template->StartFrame >> Template->EndFrame >> Template->DefaultFrame >> Template->RepeatType;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Add to list
		if(!IsServer) {
			Template->FramesPerLine = Template->Texture->Size.x / Template->FrameSize.x;
			Template->TextureScale = glm::vec2(Template->FrameSize) / glm::vec2(Template->Texture->Size);
		}
		AnimationTemplates[Name] = Template;
	}

	File.close();
}

// Loads the styles
void _Assets::LoadStyles(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Name;
		std::string BackgroundColorName;
		std::string BorderColorName;
		std::string ProgramName;
		std::string TextureName;
		std::string TextureColorName;
		std::getline(File, Name, '\t');
		std::getline(File, BackgroundColorName, '\t');
		std::getline(File, BorderColorName, '\t');
		std::getline(File, ProgramName, '\t');
		std::getline(File, TextureName, '\t');
		std::getline(File, TextureColorName, '\t');

		// Check for color
		if(BackgroundColorName != "" && Colors.find(BackgroundColorName) == Colors.end())
			throw std::runtime_error("Unable to find color: " + BackgroundColorName + " for style: " + Name);

		// Check for color
		if(BorderColorName != "" && Colors.find(BorderColorName) == Colors.end())
			throw std::runtime_error("Unable to find color: " + BorderColorName + " for style: " + Name);

		// Find program
		if(Programs.find(ProgramName) == Programs.end())
		   throw std::runtime_error("Cannot find program: " + ProgramName + " for style: " + Name);

		// Check for texture
		if(TextureName != "" && Textures.find(TextureName) == Textures.end())
			throw std::runtime_error("Unable to find texture: " + TextureName + " for style: " + Name);

		bool Stretch;
		File >> Stretch;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Get colors
		glm::vec4 BackgroundColor = Colors[BackgroundColorName];
		glm::vec4 BorderColor = Colors[BorderColorName];
		glm::vec4 TextureColor = Colors[TextureColorName];

		// Get textures
		const _Texture *Texture = Textures[TextureName];

		// Create style
		_Style *Style = new _Style();
		Style->Name = Name;
		Style->HasBackgroundColor = BackgroundColorName != "";
		Style->HasBorderColor = BorderColorName != "";
		Style->BackgroundColor = BackgroundColor;
		Style->BorderColor = BorderColor;
		Style->Program = Programs[ProgramName];
		Style->Texture = Texture;
		Style->TextureColor = TextureColor;
		Style->Stretch = Stretch;

		// Check for duplicates
		if(Styles.find(Name) != Styles.end())
			throw std::runtime_error("Duplicate style Name: " + Name);

		Styles[Name] = Style;
	}

	File.close();
}

// Load elements table
void _Assets::LoadElements(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);

		// Check for duplicates
		if(Elements.find(Identifier) != Elements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Read attributes
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		bool MaskOutside;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaskOutside;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for style
		if(StyleIdentifier != "" && Styles.find(StyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + StyleIdentifier + " for element: " + Identifier);

		// Create
		_Element *Element = new _Element();
		Element->Identifier = Identifier;
		Element->ParentIdentifier = ParentIdentifier;
		Element->Offset = Offset;
		Element->Size = Size;
		Element->Alignment = Alignment;
		Element->Style = Styles[StyleIdentifier];
		Element->MaskOutside = MaskOutside;

		// Add to map
		Element->GlobalID = AllElements.size();
		Elements[Identifier] = Element;
		AllElements[Identifier] = Element;
	}

	File.close();
}

// Load labels table
void _Assets::LoadLabels(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string FontIdentifier = GetTSVText(File);
		std::string ColorIdentifier = GetTSVText(File);
		std::string Text = GetTSVText(File);

		// Check for duplicates
		if(Labels.find(Identifier) != Labels.end())
			throw std::runtime_error("Duplicate label: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Get font
		const _Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Read attributes
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_Label *Label = new _Label();
		Label->Identifier = Identifier;
		Label->ParentIdentifier = ParentIdentifier;
		Label->Offset = Offset;
		Label->Size = Size;
		Label->Alignment = Alignment;
		Label->Font = Font;
		Label->Text = Text;
		Label->Color = Colors[ColorIdentifier];

		// Add to map
		Label->GlobalID = AllElements.size();
		Labels[Identifier] = Label;
		AllElements[Identifier] = Label;
	}

	File.close();
}

// Load buttons
void _Assets::LoadButtons(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);
		std::string HoverStyleIdentifier = GetTSVText(File);

		// Check for duplicates
		if(Buttons.find(Identifier) != Buttons.end())
			throw std::runtime_error("Duplicate button: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Check for style
		if(StyleIdentifier != "" && Styles.find(StyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + StyleIdentifier + " for button: " + Identifier);

		// Check for hover style
		if(HoverStyleIdentifier != "" && Styles.find(HoverStyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + HoverStyleIdentifier + " for button: " + Identifier);

		// Get style
		_Style *Style = Styles[StyleIdentifier];
		_Style *HoverStyle = Styles[HoverStyleIdentifier];

		// Read attributes
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		intptr_t UserData;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> UserData;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_Button *Button = new _Button();
		Button->Identifier = Identifier;
		Button->ParentIdentifier = ParentIdentifier;
		Button->Offset = Offset;
		Button->Size = Size;
		Button->Alignment = Alignment;
		Button->Style = Style;
		Button->HoverStyle = HoverStyle;
		Button->UserData = (void *)UserData;

		// Add to map
		Button->GlobalID = AllElements.size();
		Buttons[Identifier] = Button;
		AllElements[Identifier] = Button;
	}

	File.close();
}

// Load textboxes
void _Assets::LoadTextBoxes(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);
		std::string FontIdentifier = GetTSVText(File);

		// Check for duplicates
		if(TextBoxes.find(Identifier) != TextBoxes.end())
			throw std::runtime_error("Duplicate textbox: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Check for style
		if(StyleIdentifier != "" && Styles.find(StyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + StyleIdentifier + " for textbox: " + Identifier);

		// Get font
		const _Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Get style
		_Style *Style = Styles[StyleIdentifier];

		// Read attributes
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		int MaxLength;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaxLength;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_TextBox *TextBox = new _TextBox();
		TextBox->Identifier = Identifier;
		TextBox->ParentIdentifier = ParentIdentifier;
		TextBox->Offset = Offset;
		TextBox->Size = Size;
		TextBox->Alignment = Alignment;
		TextBox->Style = Style;
		TextBox->Font = Font;
		TextBox->MaxLength = MaxLength;

		// Add to map
		TextBox->GlobalID = AllElements.size();
		TextBoxes[Identifier] = TextBox;
		AllElements[Identifier] = TextBox;
	}

	File.close();
}

// Turn ParentIdentifier into Parent pointers
void _Assets::ResolveElementParents() {

	// Sort elements by global id
	std::map<size_t, _Element *> SortedElements;
	for(const auto &Iterator : AllElements) {
		_Element *Element = Iterator.second;

		SortedElements[Element->GlobalID] = Element;
	}

	// Iterate through sorted elements
	for(const auto &Iterator : SortedElements) {
		_Element *Element = Iterator.second;

		// Set parent pointer
		if(Element->ParentIdentifier != "") {
			if(AllElements.find(Element->ParentIdentifier) == AllElements.end())
				throw std::runtime_error("Cannot find parent element: " + Element->ParentIdentifier);

			Element->Parent = AllElements[Element->ParentIdentifier];
		}
		else
			Element->Parent = Graphics.Element;

		Element->Parent->Children.push_back(Element);
		Element->CalculateBounds();
	}
}

// Load the UI xml file
void _Assets::LoadUI(const std::string &Path) {

	// Load file
	tinyxml2::XMLDocument Document;
	if(Document.LoadFile(Path.c_str()) != tinyxml2::XML_SUCCESS)
		throw std::runtime_error("Error loading: " + Path);

	// Load elements
	tinyxml2::XMLElement *ChildNode = Document.FirstChildElement();
	//Graphics.Element = new _Element(ChildNode, nullptr);
	//Graphics.Element->Alignment = LEFT_TOP;
	//Graphics.Element->Active = true;
	//Graphics.Element->Size = Graphics.CurrentSize;
	//Graphics.Element->CalculateBounds();
}

// Save UI to xml
void _Assets::SaveUI(const std::string &Path) {

	// Create doc
	tinyxml2::XMLDocument Document;
	Document.InsertEndChild(Document.NewDeclaration());

	// Serialize root ui element
	//Graphics.Element->SerializeElement(Document, nullptr);

	// Write file
	Document.SaveFile(Path.c_str());
}
