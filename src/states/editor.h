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
#include <state.h>
#include <string>
#include <vector>
#include <list>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
class _Font;
class _Texture;
class _Element;
class _Button;
class _Map;
class _TextBox;
class _Camera;
class _Stats;
class _Atlas;
class _Object;

enum EditorModeType {
	EDITMODE_TILES,
	EDITMODE_BLOCKS,
	EDITMODE_OBJECTS,
	EDITMODE_PROPS,
	EDITMODE_COUNT
};

enum EditorInputTypes {
	EDITINPUT_LOAD,
	EDITINPUT_SAVE,
	EDITINPUT_COUNT
};

// Stores palette information that brushes use
struct _Palette {
	_Palette(const std::string &Identifier, const std::string &Text, void *UserData, const _Texture *Texture, const _Atlas *Atlas, int TextureIndex, const glm::vec4 &Color) :
		Identifier(Identifier),
		Text(Text),
		UserData(UserData),
		Texture(Texture),
		Atlas(Atlas),
		TextureIndex(TextureIndex),
		Color(Color) { }

	std::string Identifier;
	std::string Text;
	void *UserData;
	const _Texture *Texture;
	const _Atlas *Atlas;
	int TextureIndex;
	glm::vec4 Color;
};

// Editor state
class _EditorState : public _State {

	public:

		// Setup
		_EditorState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void TextEvent(const char *Text) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;
		void MouseWheelEvent(int Direction) override;
		void WindowEvent(uint8_t Event) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// State parameters
		void SetMapFilename(const std::string &Filename) { MapFilename = Filename; }
		void SetStats(const _Stats *Stats)  { this->Stats = Stats; }

		// Callbacks
		static void ExecuteSwitchMode(_EditorState *State, _Element *Element);
		static void ExecuteWalkable(_EditorState *State, _Element *Element);
		static void ExecuteChangeZ(_EditorState *State, _Element *Element);
		static void ExecuteDeselect(_EditorState *State, _Element *Element);
		static void ExecuteDelete(_EditorState *State, _Element *Element);
		static void ExecuteCopy(_EditorState *State, _Element *Element);
		static void ExecutePaste(_EditorState *State, _Element *Element);
		static void ExecuteHighlightBlocks(_EditorState *State, _Element *Element);
		static void ExecuteNew(_EditorState *State, _Element *Element);
		static void ExecuteIOCommand(_EditorState *State, _Element *Element);
		static void ExecuteTest(_EditorState *State, _Element *Element);
		static void ExecuteUpdateGridMode(_EditorState *State, _Element *Element);
		typedef void (*CallbackType)(_EditorState *State, _Element *Element);

	protected:

		bool LoadMap(const std::string &File, bool UseSavedCameraPosition=false);
		void ResetState();

		void DrawBrush();

		void LoadPalettes();
		void LoadPaletteButtons(const std::vector<_Palette> &Palette, int Type);
		void ClearPalette(int Type);

		void UpdateEventIdentifier(int Type, const std::string &Identifier);
		void DeselectObjects() { SelectedObjects.clear(); }
		void ClearClipboard() { ClipboardObjects.clear(); }
		bool ObjectsSelected() { return SelectedObjects.size() != 0; }

		void ConfirmMove();
		void CancelMove();

		glm::vec2 AlignToGrid(const glm::vec2 &Position) const;

		void ExecuteUpdateCheckpointIndex(int Value);
		void ExecuteSelectPalette(_Button *Button, int ClickType);

		// Parameters
		glm::vec3 SavedCameraPosition;
		int CheckpointIndex;
		std::string MapFilename;
		int SavedPalette;
		const _Stats *Stats;

		// Map editing
		_Camera *Camera;
		_Map *Map;
		glm::vec2 WorldCursor;
		float *GridVertices;
		float AlignDivisor;
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
		const _Font *MainFont;
		_Button *LayerButtons[5];
		_Button *ModeButtons[EDITMODE_COUNT];
		_Button *Brush[EDITMODE_COUNT];
		_Element *CommandElement;
		_Element *BlockElement;
		_Element *PaletteElement[EDITMODE_COUNT];
		_TextBox *InputBox;

		// Blocks
		glm::vec3 DrawStart;
		glm::vec3 DrawEnd;
		glm::vec3 OldStart;
		glm::vec3 OldEnd;
		glm::vec2 SavedIndex;
		bool HighlightBlocks;
		int Collision;

		// Objects
		std::list<_Object *> SelectedObjects;
		std::list<_Object *> ClipboardObjects;
		glm::vec2 ClickedPosition;
		glm::vec2 CopiedPosition;
};

extern _EditorState EditorState;
