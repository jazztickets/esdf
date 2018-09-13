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
#pragma once

// Libraries
#include <ae/state.h>
#include <string>
#include <vector>
#include <list>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
class _Button;
class _Map;
class _TextBox;
class _Stats;
class _Object;

namespace ae {
	template<class T> class _Manager;
	class _Font;
	class _Camera;
	class _Element;
	class _Atlas;
	class _Texture;
	struct _Style;
}

enum EditorModeType {
	EDITMODE_TILES,
	EDITMODE_BLOCKS,
	EDITMODE_OBJECTS,
	EDITMODE_PROPS,
	EDITMODE_ZONE,
	EDITMODE_COUNT
};

enum EditorInputTypes {
	EDITINPUT_LOAD,
	EDITINPUT_SAVE,
	EDITINPUT_SCRIPT,
	EDITINPUT_COUNT
};

// Stores palette information that brushes use
struct _Palette {
	_Palette(const std::string &Identifier, const std::string &Text, void *UserData, const ae::_Texture *Texture, const ae::_Atlas *Atlas, const ae::_Style *Style, uint32_t TextureIndex, const glm::vec4 &Color) :
		Identifier(Identifier),
		Text(Text),
		UserData(UserData),
		Texture(Texture),
		Atlas(Atlas),
		Style(Style),
		TextureIndex(TextureIndex),
		Color(Color) { }

	std::string Identifier;
	std::string Text;
	void *UserData;
	const ae::_Texture *Texture;
	const ae::_Atlas *Atlas;
	const ae::_Style *Style;
	uint32_t TextureIndex;
	glm::vec4 Color;
};

// Editor state
class _EditorState : public ae::_State {

	public:

		// Setup
		_EditorState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, size_t Action, int Value) override;
		void HandleKey(const ae::_KeyEvent &HandleKey) override;
		void HandleMouseButton(const ae::_MouseEvent &HandleMouseButton) override;
		void HandleMouseWheel(int Direction) override;
		void HandleWindow(uint8_t Event) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// State parameters
		void SetMapFilename(const std::string &Filename) { MapFilename = Filename; SavedText[EDITINPUT_SAVE] = Filename; }
		void SetStats(const _Stats *Stats)  { this->Stats = Stats; }

		// Callbacks
		static void ExecuteSwitchMode(_EditorState *State, ae::_Element *Element);
		static void ExecuteWalkable(_EditorState *State, ae::_Element *Element);
		static void ExecuteChangeZ(_EditorState *State, ae::_Element *Element);
		static void ExecuteDeselect(_EditorState *State, ae::_Element *Element);
		static void ExecuteDelete(_EditorState *State, ae::_Element *Element);
		static void ExecuteCopy(_EditorState *State, ae::_Element *Element);
		static void ExecutePaste(_EditorState *State, ae::_Element *Element);
		static void ExecuteHighlightBlocks(_EditorState *State, ae::_Element *Element);
		static void ExecuteNew(_EditorState *State, ae::_Element *Element);
		static void ExecuteIOCommand(_EditorState *State, ae::_Element *Element);
		static void ExecuteTest(_EditorState *State, ae::_Element *Element);
		static void ExecuteUpdateGridMode(_EditorState *State, ae::_Element *Element);
		typedef void (*CallbackType)(_EditorState *State, ae::_Element *Element);

	protected:

		bool LoadMap(const std::string &File, bool UseSavedCameraPosition=false);
		void ResetState();

		void DrawBrush();

		void LoadPalettes();
		void LoadPaletteButtons(const std::vector<_Palette> &Palette, int Type);
		void ClearPalette(int Type);

		void UpdateEventIdentifier(int Type, const std::string &Identifier);
		bool ObjectsSelected() { return SelectedObjects.size() != 0; }

		void ConfirmMove();
		void CancelMove();

		glm::vec2 AlignToGrid(const glm::vec2 &Position) const;

		void ExecuteUpdateCheckpointIndex(int Value);
		void ExecuteSelectPalette(ae::_Element *Button, int ClickType);

		// Parameters
		glm::vec3 SavedCameraPosition;
		int CheckpointIndex;
		std::string MapFilename;
		int SavedPalette;
		const _Stats *Stats;

		// Map editing
		ae::_Camera *Camera;
		_Map *Map;
		glm::vec2 WorldCursor;
		float *GridVertices;
		float AlignDivisor;
		float TileBrushRadius;
		int GridMode;
		bool IsDrawing;
		bool IsMoving;
		bool IsShiftDown;
		bool IsCtrlDown;
		bool DraggingBox;

		// Text input
		std::string SavedText[EDITINPUT_COUNT];
		int EditorInputType;
		bool IgnoreTextEvent;

		// UI
		int CurrentPalette;
		const ae::_Font *MainFont;
		ae::_Element *ModeButtons[EDITMODE_COUNT];
		ae::_Element *Brush[EDITMODE_COUNT];
		ae::_Element *CommandElement;
		ae::_Element *BlockElement;
		ae::_Element *ZoneElement;
		ae::_Element *PaletteElement[EDITMODE_COUNT];
		ae::_Element *InputBox;

		// Blocks
		glm::vec3 DrawStart;
		glm::vec3 DrawEnd;
		glm::vec3 OldStart;
		glm::vec3 OldEnd;
		glm::vec2 SavedIndex;
		bool HighlightBlocks;
		int Collision;

		// Objects
		ae::_Manager<_Object> *ObjectManager;
		std::list<_Object *> SelectedObjects;
		std::list<_Object *> ClipboardObjects;
		glm::vec2 ClickedPosition;
		glm::vec2 CopiedPosition;
};

extern _EditorState EditorState;
