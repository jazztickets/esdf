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
#include <map.h>
#include <string>
#include <vector>
#include <list>
#include <glm/vec2.hpp>

// Forward Declarations
class _Font;
class _Texture;
class _Element;
class _Button;
class _Map;
class _TextBox;
class _Camera;
class _Prop;
class _Stats;
struct _Spawn;

// Enumerations
enum EditorIconTypes {
	ICON_BASE,
	ICON_FLOOR0,
	ICON_FLOOR1,
	ICON_WALL,
	ICON_FORE,
	ICON_TILES,
	ICON_BLOCK,
	ICON_OBJECTS,
	ICON_PROPS,
	ICON_NONE,
	ICON_DELETE,
	ICON_COPY,
	ICON_PASTE,
	ICON_SHOW,
	ICON_CLEAR,
	ICON_GRID,
	ICON_MSET,
	ICON_LOAD,
	ICON_SAVE,
	ICON_TEST
};

enum EditorModeType {
	EDITMODE_TILES,
	EDITMODE_BLOCKS,
	EDITMODE_OBJECTS,
	EDITMODE_PROPS,
	EDITMODE_COUNT
};

enum EditorBlockIconTypes {
	ICON_WALK,
	ICON_RAISE,
	ICON_LOWER
};

enum EditorInputTypes {
	EDITINPUT_LOAD,
	EDITINPUT_SAVE,
	EDITINPUT_COUNT
};

// Stores palette information that brushes use
struct _Palette {
	_Palette(const std::string &Identifier, const std::string &Text, const _Texture *Texture, const _Atlas *Atlas, int TextureIndex, const glm::vec4 &Color) :
		Identifier(Identifier),
		Text(Text),
		Texture(Texture),
		Atlas(Atlas),
		TextureIndex(TextureIndex),
		Color(Color) { }

	std::string Identifier;
	std::string Text;
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

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// State parameters
		void SetMapFilename(const std::string &Filename) { MapFilename = Filename; }
		void SetStats(const _Stats *Stats)  { this->Stats = Stats; }

	protected:

		bool LoadMap(const std::string &File, bool UseSavedCameraPosition=false);
		void ResetState();

		void DrawObject(float OffsetX, float OffsetY, const _Spawn *Object, float Alpha) const;
		void DrawProp(float OffsetX, float OffsetY, const _Prop *Prop, float Alpha) const;
		void DrawBrush();
		void ProcessIcons(int Index, int Type);
		void ProcessBlockIcons(int Index, int Type);

		void LoadPalettes();
		void LoadPaletteButtons(const std::vector<_Palette> &Palette, int Type);
		void ClearPalette(int Type);

		void UpdateEventIdentifier(int Type, const std::string &Identifier);
		void SpawnObject(const glm::vec2 &Position, const std::string &Identifier, bool Align);
		void SpawnProp(const glm::vec2 &Position, const std::string &Identifier, bool Align);
		void SelectObject();
		void SelectObjects();
		void DeselectBlock() { SelectedBlock = nullptr; }
		void DeselectObjects();
		void ClearClipboard();
		bool BlockSelected() { return SelectedBlock != nullptr; }
		bool ObjectsSelected() { return SelectedObjects.size() != 0; }

		bool ObjectInSelectedList(_Spawn *Object);
		glm::vec2 AlignToGrid(const glm::vec2 &Position) const;
		glm::vec2 GetMoveDeltaPosition(const glm::vec2 &Position);

		void ExecuteWalkable();
		void ExecuteToggleTile();
		void ExecuteIOCommand(int Type);
		void ExecuteClear();
		void ExecuteTest();
		void ExecuteDelete();
		void ExecuteCopy();
		void ExecutePaste(bool Viewport);
		void ExecuteDeselect();
		void ExecuteChangeZ(float Change, int Type);
		void ExecuteUpdateCheckpointIndex(int Value);
		void ExecuteSelectPalette(_Button *Button, int ClickType);
		void ExecuteUpdateSelectedPalette(int Change);
		void ExecuteUpdateGridMode(int Change);
		void ExecuteHighlightBlocks();
		void ExecuteSwitchMode(int State);
		void ExecuteUpdateBlockLimits(int Direction, bool Expand);

		// Parameters
		glm::vec2 SavedCameraPosition;
		int CheckpointIndex;
		std::string MapFilename;
		int SavedPalette;
		const _Stats *Stats;

		// Map editing
		_Camera *Camera;
		_Map *Map;
		glm::vec2 WorldCursor;
		float AlignDivisor;
		int GridMode;
		bool IsDrawing;
		bool IsMoving;
		bool IsShiftDown;
		bool IsCtrlDown;
		bool DraggingBox;

		// Text input
		std::string SavedText[EDITINPUT_COUNT];
		int EditorInput;
		bool BlockTextEvent;

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
		_Block *SelectedBlock;
		_Block ClipboardBlock;
		glm::vec3 DrawStart;
		glm::vec3 DrawEnd;
		glm::vec3 OldStart;
		glm::vec3 OldEnd;
		glm::vec2 SavedIndex;
		bool FinishedDrawing;
		bool HighlightBlocks;
		int Collision;
		bool BlockCopied;

		// Objects
		std::list<_Spawn *> SelectedObjects;
		std::list<_Spawn *> ClipboardObjects;
		std::list<size_t> SelectedObjectIndices;
		glm::vec2 ClickedPosition;
		glm::vec2 CopiedPosition;
		glm::vec2 MoveDelta;
};

extern _EditorState EditorState;
