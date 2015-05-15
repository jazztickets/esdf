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
#include <states/editor.h>
#include <states/client.h>
#include <objects/object.h>
#include <objects/prop.h>
#include <ui/element.h>
#include <ui/button.h>
#include <ui/textbox.h>
#include <ui/label.h>
#include <ui/style.h>
#include <framework.h>
#include <graphics.h>
#include <stats.h>
#include <camera.h>
#include <input.h>
#include <font.h>
#include <assets.h>
#include <map.h>
#include <menu.h>
#include <texture.h>
#include <atlas.h>
#include <config.h>
#include <constants.h>
#include <files.h>
#include <mesh.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <SDL_keycode.h>
#include <SDL_mouse.h>

#include <program.h>
#include <glm/gtc/type_ptr.hpp>

_EditorState EditorState;

// Input box
const char *InputBoxStrings[EDITINPUT_COUNT] = {
	"Load map",
	"Save map",
};

// Input box
const int PaletteSizes[EDITMODE_COUNT] = {
	64,
	64,
	64,
	64,
};

// Constructor
_EditorState::_EditorState()
:	SavedCameraPosition(0),
	CheckpointIndex(0),
	MapFilename(""),
	SavedPalette(-1) {

}

void _EditorState::Init() {

	// Load command buttons
	MainFont = Assets.Fonts["hud_medium"];
	CommandElement = Assets.Elements["element_editor_command"];
	BlockElement = Assets.Elements["element_editor_blocks"];
	InputBox = Assets.TextBoxes["textbox_editor_input"];

	// Create button groups
	PaletteElement[0] = Assets.Elements["element_editor_palette_tiles"];
	PaletteElement[1] = Assets.Elements["element_editor_palette_block"];
	PaletteElement[2] = Assets.Elements["element_editor_palette_objects"];
	PaletteElement[3] = Assets.Elements["element_editor_palette_props"];

	// Assign palette buttons
	ModeButtons[0] = Assets.Buttons["button_editor_mode_tiles"];
	ModeButtons[1] = Assets.Buttons["button_editor_mode_block"];
	ModeButtons[2] = Assets.Buttons["button_editor_mode_objects"];
	ModeButtons[3] = Assets.Buttons["button_editor_mode_props"];

	// Assign layer buttons
	LayerButtons[0] = Assets.Buttons["button_editor_layer_base"];
	LayerButtons[1] = Assets.Buttons["button_editor_layer_floor0"];
	LayerButtons[2] = Assets.Buttons["button_editor_layer_floor1"];
	LayerButtons[3] = Assets.Buttons["button_editor_layer_wall"];
	LayerButtons[4] = Assets.Buttons["button_editor_layer_fore"];

	// Reset state
	ResetState();

	// Create camera
	Camera = new _Camera(glm::vec2(0), CAMERA_DISTANCE, CAMERA_EDITOR_DIVISOR);

	// Load level
	if(ClientState.GetFromEditor())
		MapFilename = EDITOR_TESTLEVEL;

	LoadMap(MapFilename, ClientState.GetFromEditor());

	// Set up graphics
	Graphics.ChangeViewport(Graphics.WindowSize - EDITOR_VIEWPORT_OFFSET);
	Camera->CalculateFrustum(Graphics.AspectRatio);
	Graphics.ShowCursor(true);

	if(SavedPalette != -1)
		ExecuteSwitchMode(SavedPalette);
};

void _EditorState::Close() {
	SavedCameraPosition = Camera->GetPosition();
	SavedPalette = CurrentPalette;

	for(int i = 0; i < EDITMODE_COUNT; i++)
		ClearPalette(i);

	delete Camera;
	delete Map;

	Camera = nullptr;
	Map = nullptr;
};

// Load a level
bool _EditorState::LoadMap(const std::string &File, bool UseSavedCameraPosition) {
	if(Map)
		delete Map;

	Map = new _Map(File, Stats);
	Map->SetCamera(Camera);
	ResetState();

	// Set camera
	if(UseSavedCameraPosition)
		Camera->ForcePosition(SavedCameraPosition);
	else
		Camera->ForcePosition(Map->GetStartingPositionByCheckpoint(0));

	// Load tileset
	std::vector<_Palette> Palette;
	int TextureCount = Map->TileAtlas->Texture->Size.x * Map->TileAtlas->Texture->Size.y / (Map->TileAtlas->Size.x * Map->TileAtlas->Size.y);
	for(int i = 0; i < TextureCount; i++) {
		Palette.push_back(_Palette(std::to_string(i), std::to_string(i), nullptr, Map->TileAtlas, i, COLOR_WHITE));
	}

	LoadPaletteButtons(Palette, EDITMODE_TILES);

	return true;
}

void _EditorState::ResetState() {
	WorldCursor = glm::vec2(0);
	SelectedBlock = nullptr;
	EditorInput = -1;
	CheckpointIndex = 0;
	ClickedPosition = glm::vec2(0);
	CopiedPosition = glm::vec2(0);
	MoveDelta = glm::vec2(0);
	CurrentPalette = EDITMODE_TILES;
	GridMode = EDITOR_DEFAULT_GRIDMODE;
	Collision = 0;
	HighlightBlocks = false;
	SelectedObjects.clear();
	SelectedObjectIndices.clear();
	ClipboardObjects.clear();

	AlignDivisor = EDITOR_ALIGN_DIVISOR;
	IsShiftDown = false;
	IsCtrlDown = false;
	DraggingBox = false;
	BlockCopied = false;
	IsDrawing = false;
	IsMoving = false;
	FinishedDrawing = false;
	BlockTextEvent = false;

	DrawStart.x = 0;
	DrawStart.y = 0;
	DrawEnd.x = 0;
	DrawEnd.y = 0;
	DrawEnd.z = MAP_WALLZ;
	OldStart.x = 0;
	OldStart.y = 0;
	OldEnd.x = 0;
	OldEnd.y = 0;
	SavedIndex.x = 0;
	SavedIndex.y = 0;

	// Enable default button
	for(int i = 0; i < 5; i++)
		LayerButtons[i]->Enabled = false;

	for(int i = 0; i < EDITMODE_COUNT; i++) {
		ModeButtons[i]->Enabled = false;
		Brush[i] = nullptr;
	}

	// Load palettes
	LoadPalettes();
	LayerButtons[0]->Enabled = true;
	ModeButtons[CurrentPalette]->Enabled = true;
}

// Action handler
bool _EditorState::HandleAction(int InputType, int Action, int Value) {

	return false;
}

// Key handler
void _EditorState::KeyEvent(const _KeyEvent &KeyEvent) {
	if(IsMoving || IsDrawing || !KeyEvent.Pressed)
		return;

	// See if the user is entering in text
	if(EditorInput != -1) {
		switch(KeyEvent.Key) {
			case SDL_SCANCODE_RETURN: {
				const std::string InputText = InputBox->Text;
				switch(EditorInput) {
					case EDITINPUT_LOAD: {
						if(InputText == "")
							break;

						if(LoadMap(InputText, false))
							SavedText[EDITINPUT_SAVE] = InputText;

					} break;
					case EDITINPUT_SAVE:
						if(InputText == "" || !Map->Save(InputText))
							SavedText[EditorInput] = "";
						else {
							SavedText[EditorInput] = InputText;
						}

						ExecuteDeselect();
					break;
				}
				EditorInput = -1;
			} break;
			case SDL_SCANCODE_ESCAPE:
				EditorInput = -1;
			break;
			default:
				InputBox->HandleKeyEvent(KeyEvent);
			break;
		}
	}
	else {

		// Command keys
		switch(KeyEvent.Key) {

			// Exit
			case SDL_SCANCODE_ESCAPE:
				Framework.SetDone(true);
			break;
			/*
			case SDL_SCANCODE_F1:
			break;
			case SDL_SCANCODE_F2:
			break;
			case SDL_SCANCODE_F3:
			break;
			case SDL_SCANCODE_F4:
			break;
			case SDL_SCANCODE_W:
			break;
			case SDL_SCANCODE_F:
			break;
			*/
			case SDL_SCANCODE_MINUS:
				ExecuteUpdateCheckpointIndex(-1);
			break;
			case SDL_SCANCODE_EQUALS:
				ExecuteUpdateCheckpointIndex(1);
			break;
			case SDL_SCANCODE_1:
				ExecuteSwitchMode(EDITMODE_TILES);
			break;
			case SDL_SCANCODE_2:
				ExecuteSwitchMode(EDITMODE_BLOCKS);
			break;
			case SDL_SCANCODE_3:
				ExecuteSwitchMode(EDITMODE_OBJECTS);
			break;
			case SDL_SCANCODE_4:
				ExecuteSwitchMode(EDITMODE_PROPS);
			break;
			case SDL_SCANCODE_GRAVE:
				ExecuteDeselect();
			break;
			case SDL_SCANCODE_D:
				ExecuteDelete();
			break;
			case SDL_SCANCODE_C:
				ExecuteCopy();
			break;
			case SDL_SCANCODE_V:
				ExecutePaste(true);
			break;
			case SDL_SCANCODE_G:
				if(IsShiftDown)
					ExecuteUpdateGridMode(-1);
				else
					ExecuteUpdateGridMode(1);
			break;
			case SDL_SCANCODE_B:
				ExecuteHighlightBlocks();
			break;
			case SDL_SCANCODE_A:
				ExecuteWalkable();
			break;
			case SDL_SCANCODE_KP_MINUS:
				ExecuteChangeZ(-0.5f, !IsShiftDown);
			break;
			case SDL_SCANCODE_KP_PLUS:
				ExecuteChangeZ(0.5f, !IsShiftDown);
			break;
			case SDL_SCANCODE_N:
				if(IsCtrlDown)
					ExecuteClear();
			break;
			case SDL_SCANCODE_L:
				ExecuteIOCommand(EDITINPUT_LOAD);
				BlockTextEvent = true;
			break;
			case SDL_SCANCODE_S:
				ExecuteIOCommand(EDITINPUT_SAVE);
				BlockTextEvent = true;
			break;
			case SDL_SCANCODE_T:
				ExecuteTest();
			break;
			case SDL_SCANCODE_TAB:
				if(IsShiftDown)
					ExecuteUpdateSelectedPalette(-1);
				else
					ExecuteUpdateSelectedPalette(1);
			break;
			case SDL_SCANCODE_LEFT:
				ExecuteUpdateBlockLimits(0, !IsShiftDown);
			break;
			case SDL_SCANCODE_UP:
				ExecuteUpdateBlockLimits(1, !IsShiftDown);
			break;
			case SDL_SCANCODE_RIGHT:
				ExecuteUpdateBlockLimits(2, !IsShiftDown);
			break;
			case SDL_SCANCODE_DOWN:
				ExecuteUpdateBlockLimits(3, !IsShiftDown);
			break;
		}
	}
};

// Text event handler
void _EditorState::TextEvent(const char *Text) {
	if(EditorInput != -1) {
		if(BlockTextEvent)
			BlockTextEvent = false;
		else
			InputBox->HandleTextEvent(Text);
	}
}

// Mouse handler
void _EditorState::MouseEvent(const _MouseEvent &MouseEvent) {

	if(MouseEvent.Button == SDL_BUTTON_LEFT) {
		CommandElement->HandleInput(MouseEvent.Pressed);
		BlockElement->HandleInput(MouseEvent.Pressed);
	}
	if(MouseEvent.Button == SDL_BUTTON_LEFT || MouseEvent.Button == SDL_BUTTON_RIGHT) {
		PaletteElement[CurrentPalette]->HandleInput(MouseEvent.Pressed);
	}

	// Handle command group clicks
	_Element *Clicked = CommandElement->GetClickedElement();
	if(Clicked && Clicked->ID != -1) {
		ProcessIcons(Clicked->ID, MouseEvent.Button == SDL_BUTTON_RIGHT);
	}

	if(CurrentPalette == EDITMODE_BLOCKS) {
		_Element *Clicked = BlockElement->GetClickedElement();
		if(Clicked && Clicked->ID != -1) {
			ProcessBlockIcons(Clicked->ID, MouseEvent.Button == SDL_BUTTON_RIGHT);
		}
	}

	// Distinguish between interface and viewport clicks
	if(Input.GetMouse().x < Graphics.ViewportSize.x && Input.GetMouse().y < Graphics.ViewportSize.y) {
		if(MouseEvent.Pressed) {

			// Mouse press
			switch(MouseEvent.Button) {
				case SDL_BUTTON_LEFT:
					if(!IsMoving && !Clicked) {
						_Button *Button = Brush[CurrentPalette];

						switch(CurrentPalette) {
							case EDITMODE_TILES:
								IsDrawing = true;
								FinishedDrawing = false;
							break;
							case EDITMODE_BLOCKS:
								DeselectBlock();

								// Save the indices
								SavedIndex = WorldCursor;
								IsDrawing = true;
								FinishedDrawing = false;
							break;
							case EDITMODE_OBJECTS:
								if(Button)
									SpawnObject(Map->GetValidPosition(WorldCursor), Button->Identifier, IsShiftDown);
							break;
							case EDITMODE_PROPS:
								if(Button)
									SpawnProp(Map->GetValidPosition(WorldCursor), Button->Identifier, IsShiftDown);
							break;
						}
					}
				break;
				// Move the camera
				case SDL_BUTTON_RIGHT:
					Camera->SetPosition(WorldCursor);
				break;
				// Select object
				case SDL_BUTTON_MIDDLE:
					if(!IsDrawing) {
						switch(CurrentPalette) {
							case EDITMODE_BLOCKS:

								// Get the block
								Map->GetSelectedBlock(WorldCursor, &SelectedBlock);
								if(BlockSelected()) {

									// Save old states
									OldStart = SelectedBlock->Start;
									OldEnd = SelectedBlock->End;
									SavedIndex = WorldCursor;

									IsMoving = true;
								}

							break;
							default:
								SelectObject();
							break;
						}
					}
				break;
			}


		}
	}
	else {

		// Get button click for palette
		_Button *Button = (_Button *)PaletteElement[CurrentPalette]->GetClickedElement();
		if(Button) {
			ExecuteSelectPalette(Button, MouseEvent.Button == SDL_BUTTON_RIGHT);
		}
	}

	// Mouse release
	if(!MouseEvent.Pressed) {
		switch(MouseEvent.Button) {
			case SDL_BUTTON_LEFT:
				if(IsDrawing) {
					FinishedDrawing = true;
				}
			break;
			case SDL_BUTTON_MIDDLE:
				if(IsMoving) {
					IsMoving = false;
					for(auto Iterator : SelectedObjects) {
						Iterator->Position = GetMoveDeltaPosition(Iterator->Position);
					}
					MoveDelta = glm::vec2(0);
				}

				if(DraggingBox) {
					DraggingBox = false;
					SelectObjects();
				}
			break;
		}
	}
};

// Mouse wheel handler
void _EditorState::MouseWheelEvent(int Direction) {

	if(Input.GetMouse().x < Graphics.ViewportSize.x && Input.GetMouse().y < Graphics.ViewportSize.y) {
		float Multiplier = 1.0f * Direction;
		if(IsShiftDown)
			Multiplier = 10.0f * Direction;

		// Zoom
		Camera->UpdateDistance(-Multiplier);
	}
	else {
		if(Direction > 0)
			PaletteElement[CurrentPalette]->UpdateChildrenOffset(glm::ivec2(0, PaletteSizes[CurrentPalette]));
		else
			PaletteElement[CurrentPalette]->UpdateChildrenOffset(glm::ivec2(0, -PaletteSizes[CurrentPalette]));
	}
}

// Update
void _EditorState::Update(double FrameTime) {

	CommandElement->Update(FrameTime, Input.GetMouse());
	BlockElement->Update(FrameTime, Input.GetMouse());
	PaletteElement[CurrentPalette]->Update(FrameTime, Input.GetMouse());
	if(EditorInput != -1) {
		InputBox->Update(FrameTime, Input.GetMouse());
	}

	// Get modifier key status
	IsShiftDown = Input.ModKeyDown(KMOD_SHIFT) ? true : false;
	IsCtrlDown = Input.ModKeyDown(KMOD_CTRL) ? true : false;

	if(IsShiftDown)
		AlignDivisor = 2;
	else
		AlignDivisor = EDITOR_ALIGN_DIVISOR;

	// Get world cursor
	Camera->ConvertScreenToWorld(Input.GetMouse(), WorldCursor);

	// Get tile indices for later usage
	WorldCursor = AlignToGrid(Map->GetValidPosition(WorldCursor));

	// Set camera position
	Camera->Update(FrameTime);

	// Drawing a block
	if(IsDrawing) {

		// Get start positions
		DrawStart = glm::vec3(SavedIndex, DrawStart.z);

		// Check bounds
		DrawEnd = glm::vec3(WorldCursor, DrawEnd.z);

		// Reverse X
		if(DrawEnd.x <= DrawStart.x) {
			std::swap(DrawStart.x, DrawEnd.x);
		}

		// Reverse Y
		if(DrawEnd.y <= DrawStart.y) {
			std::swap(DrawStart.y, DrawEnd.y);
		}

		// Set minimum size
		if(std::abs(DrawEnd.x - DrawStart.x) < EDITOR_MIN_BLOCK_SIZE)
			DrawEnd.x = DrawStart.x + EDITOR_MIN_BLOCK_SIZE;

		if(std::abs(DrawEnd.y - DrawStart.y) < EDITOR_MIN_BLOCK_SIZE)
			DrawEnd.y = DrawStart.y + EDITOR_MIN_BLOCK_SIZE;
	}

	// Moving a block
	if(IsMoving) {

		// Get offsets
		glm::vec2 Offset = WorldCursor - SavedIndex;

		// Check x bounds
		if(Offset.x + OldStart.x < 0)
			Offset.x = -OldStart.x;
		else if(Offset.x + OldEnd.x >= Map->Size.x)
			Offset.x = Map->Size.x - OldEnd.x;

		// Check y bounds
		if(Offset.y + OldStart.y < 0)
			Offset.y = -OldStart.y;
		else if(Offset.y + OldEnd.y >= Map->Size.y)
			Offset.y = Map->Size.y - OldEnd.y;

		// Get start positions
		DrawStart = OldStart + glm::vec3(Offset, 0.0f);

		// Check bounds
		DrawEnd.x = OldEnd.x + Offset.x;
		DrawEnd.y = OldEnd.y + Offset.y;
		DrawEnd.z = OldEnd.z;
	}

	// Update based on editor state
	switch(CurrentPalette) {
		case EDITMODE_TILES:
			if(IsDrawing) {
				if(Brush[EDITMODE_TILES]) {
					glm::ivec2 TilePosition = Map->GetValidCoord(WorldCursor);
					Map->GetTiles()[TilePosition.x][TilePosition.y].TextureIndex = Brush[EDITMODE_TILES]->TextureIndex;
				}

				if(FinishedDrawing)
					FinishedDrawing = IsDrawing = false;
			}
		break;
		case EDITMODE_BLOCKS:

			// Finish drawing a block and add it to the list
			if(FinishedDrawing) {
				if(Brush[EDITMODE_BLOCKS]) {
					_Block Block;
					Block.Start = DrawStart;
					Block.End = DrawEnd;
					//Block.End.z++;
					Block.Texture = Brush[EDITMODE_BLOCKS]->Style->Texture;
					Block.Collision = 0;

					Map->AddBlock(Block);
				}

				FinishedDrawing = IsDrawing = false;
			}

			if(IsMoving) {
				SelectedBlock->Start = DrawStart;
				SelectedBlock->End = DrawEnd;
				//SelectedBlock->End.z++;
			}
		break;
		default:
			if(IsMoving)
				MoveDelta = WorldCursor - ClickedPosition;
		break;
	}
};

// Render the state
void _EditorState::Render(double BlendFactor) {
	glm::vec4 AmbientLight(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 LightPosition(0, 0, -100.0f);

	Assets.Programs["pos_uv"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv"]->AmbientLight = AmbientLight;
	Assets.Programs["pos_uv_norm"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLight;

	// Setup 3D transformation
	Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw floors
	Map->RenderFloors();

	// Draw walls
	Map->RenderWalls();

	// Draw props
	Map->RenderProps();

	// Draw objects
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	Graphics.SetVBO(VBO_QUAD);
	Graphics.SetDepthMask(false);
	for(auto ObjectSpawn : Map->ObjectSpawns) {
		DrawObject(0.0f, 0.0f, ObjectSpawn, 1.0f);
	}
	Graphics.SetDepthMask(true);

	// Draw faded items while moving
	Graphics.SetDepthTest(false);
	for(auto Iterator : SelectedObjects) {
		DrawObject(MoveDelta.x, MoveDelta.y, Iterator, 0.5f);
	}

	// Outline selected item
	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetVBO(VBO_CIRCLE);
	for(auto Iterator : SelectedObjects) {
		glm::vec2 Position = GetMoveDeltaPosition(Iterator->Position);
		Graphics.DrawCircle(glm::vec3(Position, ITEM_Z + 0.05f), EDITOR_OBJECTRADIUS, COLOR_WHITE);
	}

	Graphics.SetDepthTest(true);

	// Draw tentative asset
	switch(CurrentPalette) {
		case EDITMODE_TILES:
		break;
		case EDITMODE_BLOCKS:
			if(IsDrawing && Brush[CurrentPalette]) {
				Graphics.SetProgram(Assets.Programs["pos_uv"]);
				Graphics.SetVBO(VBO_CUBE);
				Graphics.DrawCube(glm::vec3(DrawStart), glm::vec3(DrawEnd - DrawStart), Brush[CurrentPalette]->Style->Texture);
			}
		break;
		case EDITMODE_OBJECTS:
			if(Brush[CurrentPalette]) {
				Graphics.SetProgram(Assets.Programs["pos_uv"]);
				Graphics.SetVBO(VBO_QUAD);
				_Spawn TempSpawn;
				TempSpawn.Identifier = Brush[CurrentPalette]->Identifier;
				TempSpawn.Position = WorldCursor;
				DrawObject(0.0f, 0.0f, &TempSpawn, 0.5f);
			}
		break;
		case EDITMODE_PROPS:
			if(Brush[CurrentPalette]) {
				const auto &Iterator = Stats->Props.find(Brush[CurrentPalette]->Identifier);
				if(Iterator != Stats->Props.end()) {
					const _PropStat &PropStat = Iterator->second;

					_Prop Prop(PropStat);
					Prop.Mesh = Assets.Meshes[PropStat.MeshIdentifier];
					Prop.Program = Assets.Programs["pos_uv_norm"];
					Prop.Texture = Assets.Textures[PropStat.TextureIdentifier];
					Prop.Position = glm::vec3(WorldCursor, 0.0f);
					DrawProp(0.0f, 0.0f, &Prop, 0.5f);
				}
			}
		break;
	}

	Graphics.SetDepthTest(false);
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetVBO(VBO_NONE);

	// Draw map boundaries
	Graphics.DrawRectangle(
				glm::vec2(-0.01f, -0.01f),
				glm::vec2(Map->Size.x + 0.01f, Map->Size.y + 0.01f),
				COLOR_RED);

	// Draw grid
	Map->RenderGrid(GridMode);

	// Outline the blocks
	if(HighlightBlocks)
		Map->HighlightBlocks();

	// Outline selected block
	if(BlockSelected())
		Graphics.DrawRectangle(glm::vec2(SelectedBlock->Start), glm::vec2(SelectedBlock->End), COLOR_WHITE);

	// Dragging a box around object
	if(DraggingBox)
		Graphics.DrawRectangle(ClickedPosition, WorldCursor, COLOR_WHITE);

	// Draw a block
	if(IsDrawing && CurrentPalette == EDITMODE_BLOCKS)
		Graphics.DrawRectangle(glm::vec2(DrawStart), glm::vec2(DrawEnd), COLOR_GREEN);

	// Setup 2D transformation
	Graphics.Setup2D();

	// Draw viewport outline
	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.DrawRectangle(glm::vec2(0, 0), glm::vec2(Graphics.ViewportSize.x, Graphics.ViewportSize.y), COLOR_DARK);

	// Draw text
	if(EditorInput != -1) {
		InputBox->Render();
	}

	int X = 16;
	int Y = (float)Graphics.WindowSize.y - 25;

	Graphics.SetProgram(Assets.Programs["text"]);
	Graphics.SetVBO(VBO_NONE);

	std::ostringstream Buffer;
	Buffer << Map->Filename;
	MainFont->DrawText(Buffer.str(), glm::vec2(25, 25));
	Buffer.str("");

	Buffer << std::fixed << std::setprecision(1) << WorldCursor.x << ", " << WorldCursor.y;
	MainFont->DrawText(Buffer.str(), glm::vec2(X, (float)Graphics.ViewportSize.y - 25));
	Buffer.str("");

	X = (float)Graphics.ViewportSize.x - 45;

	Y = (float)25;
	Buffer << Graphics.FramesPerSecond;
	MainFont->DrawText("FPS:", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	Y = (float)Graphics.ViewportSize.y - 40;
	Buffer << CheckpointIndex;
	MainFont->DrawText("Checkpoint:", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	Y += 15;
	Buffer << GridMode;
	MainFont->DrawText("Grid:", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	// Draw command buttons
	CommandElement->Render();
	if(CurrentPalette == EDITMODE_BLOCKS) {
		BlockElement->Render();
	}

	// Draw current brush
	DrawBrush();

	// Draw Palette
	PaletteElement[CurrentPalette]->Render();
};

// Load palette buttons
void _EditorState::LoadPalettes() {

	// Load block textures
	{
		std::vector<_Palette> Palette;
		_Files Files(TEXTURES_PATH + TEXTURES_BLOCKS);
		for(const auto &File : Files.Nodes) {
			std::string Identifier = TEXTURES_BLOCKS + File;
			Palette.push_back(_Palette(Identifier, Identifier, Assets.Textures[Identifier], nullptr, 0, COLOR_WHITE));
		}

		LoadPaletteButtons(Palette, EDITMODE_BLOCKS);
	}

	{
		// Load objects
		std::vector<_Palette> Palette;
		for(auto Object : Stats->Objects) {
			if(Object.second.RendersStat)
				Palette.push_back(_Palette(Object.second.Identifier, Object.second.Name, Assets.Textures[Object.second.RendersStat->Icon], nullptr, 0, COLOR_WHITE));
		}

		LoadPaletteButtons(Palette, EDITMODE_OBJECTS);
	}

	{
		// Props
		std::vector<_Palette> Palette;
		for(auto Prop : Stats->Props) {
			Palette.push_back(_Palette(Prop.second.Identifier, Prop.second.Identifier, Assets.Textures[Prop.second.TextureIdentifier], nullptr, 0, COLOR_WHITE));
		}

		LoadPaletteButtons(Palette, EDITMODE_PROPS);
	}
}

// Free memory used by palette
void _EditorState::ClearPalette(int Type) {
	std::vector<_Element *> &Children = PaletteElement[Type]->GetChildren();
	for(size_t i = 0; i < Children.size(); i++) {
		delete Children[i]->Style;
		delete Children[i];
	}
	Children.clear();
}

// Loads the palette
void _EditorState::LoadPaletteButtons(const std::vector<_Palette> &Palette, int Type) {
	ClearPalette(Type);

	// Loop through textures
	glm::ivec2 Offset(0, 0);
	int Width = PaletteElement[Type]->Size.x;
	for(size_t i = 0; i < Palette.size(); i++) {

		// Create style
		_Style *Style = new _Style;
		Style->Identifier = Palette[i].Text;
		Style->HasBackgroundColor = false;
		Style->HasBorderColor = false;
		Style->BackgroundColor = COLOR_WHITE;
		Style->BorderColor = COLOR_WHITE;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->Texture = Palette[i].Texture;
		Style->Atlas = Palette[i].Atlas;
		Style->TextureColor = Palette[i].Color;
		Style->Stretch = true;

		// Add palette button
		_Button *Button = new _Button();
		Button->Identifier = Palette[i].Identifier;
		Button->Parent = PaletteElement[Type];
		Button->Offset = Offset;
		Button->Size = glm::ivec2(PaletteSizes[Type], PaletteSizes[Type]);
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->HoverStyle = Assets.Styles["style_editor_selected0"];
		Button->UserData = (void *)1;
		PaletteElement[Type]->AddChild(Button);

		// Assign texture index for atlases
		Button->TextureIndex = Palette[i].TextureIndex;

		// Update position
		Offset.x += PaletteSizes[Type];
		if(Offset.x > Width - PaletteSizes[Type]) {
			Offset.y += PaletteSizes[Type];
			Offset.x = 0;
		}
	}

	PaletteElement[Type]->CalculateBounds();
}

// Draws the current brush
void _EditorState::DrawBrush() {

	// Get selected palette
	std::string IconText = "", IconIdentifier = "";
	glm::vec4 IconColor = COLOR_WHITE;
	const _Texture *IconTexture = nullptr;
	const _Atlas *IconAtlas = nullptr;
	int IconTextureIndex = 0;
	if(Brush[CurrentPalette]) {
		IconIdentifier = Brush[CurrentPalette]->Identifier;
		if(Brush[CurrentPalette]->Style) {
			IconText = Brush[CurrentPalette]->Style->Identifier;
			IconTexture = Brush[CurrentPalette]->Style->Texture;
			IconAtlas = Brush[CurrentPalette]->Style->Atlas;
			IconTextureIndex = Brush[CurrentPalette]->TextureIndex;
			IconColor = Brush[CurrentPalette]->Style->TextureColor;
		}
	}

	Graphics.SetProgram(Assets.Programs["text"]);
	Graphics.SetVBO(VBO_NONE);

	// Edit mode specific text
	switch(CurrentPalette) {
		case EDITMODE_BLOCKS: {

			// See if there's a selected block
			float BlockMinZ, BlockMaxZ;
			if(BlockSelected()) {
				IconTexture = SelectedBlock->Texture;
				IconText = SelectedBlock->Texture->Identifier;
				BlockMinZ = SelectedBlock->Start.z;
				BlockMaxZ = SelectedBlock->End.z;
			}
			else {
				if(Brush[CurrentPalette])
					IconText = Brush[CurrentPalette]->Identifier;
				BlockMinZ = DrawStart.z;
				BlockMaxZ = DrawEnd.z;
			}
			IconIdentifier = "";

			int X = (float)Graphics.ViewportSize.x + 100;
			int Y = (float)Graphics.ViewportSize.y + 5;

			std::ostringstream Buffer;
			Buffer << BlockMinZ;
			MainFont->DrawText("Min Z:", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");

			Buffer << BlockMaxZ;
			MainFont->DrawText("Max Z:", glm::vec2(X + 85, Y), COLOR_WHITE, RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 90, Y));
			Buffer.str("");
		} break;
		default:

			// See if there's a selected object
			if(SelectedObjects.size() > 0) {
				auto Iterator = SelectedObjects.begin();
				IconIdentifier = (*Iterator)->Identifier;
				IconText = "";
				IconTexture = nullptr;
			}

		break;
	}

	// Bottom information box
	if(IconText != "")
		MainFont->DrawText(IconText, glm::vec2(Graphics.ViewportSize) + glm::vec2(112, 130), COLOR_WHITE, CENTER_MIDDLE);

	if(IconIdentifier != "")
		MainFont->DrawText(IconIdentifier, glm::vec2(Graphics.ViewportSize) + glm::vec2(112, 145), COLOR_WHITE, CENTER_MIDDLE);

	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetVBO(VBO_NONE);

	_Bounds Bounds;
	Bounds.Start = Graphics.WindowSize - glm::ivec2(112, 84) - glm::ivec2(32);
	Bounds.End = Bounds.Start + glm::ivec2(64);

	if(IconTexture)
		Graphics.DrawImage(Bounds, IconTexture, IconColor);
	else if(IconAtlas)
		Graphics.DrawAtlas(Bounds, IconAtlas->Texture, IconAtlas->GetTextureCoords(IconTextureIndex), IconColor);
}

// Draws an object
void _EditorState::DrawObject(float OffsetX, float OffsetY, const _Spawn *Object, float Alpha) const {

	// Find object in stats map
	const auto &Iterator = Stats->Objects.find(Object->Identifier);
	if(Iterator == Stats->Objects.end())
		return;

	// Get object stats
	const _ObjectStat &ObjectStat = Iterator->second;

	// Check for a render component
	if(!ObjectStat.RendersStat)
		return;

	// Get icon texture
	const _Texture *Texture = Assets.Textures[ObjectStat.RendersStat->Icon];

	// Check if object is in view
	glm::vec2 DrawPosition(Object->Position.x + OffsetX, Object->Position.y + OffsetY);
	float Scale = ObjectStat.RendersStat->Scale;
	if(!Camera->IsCircleInView(DrawPosition, Scale))
		return;

	// Draw icon
	glm::vec4 Color(COLOR_WHITE);
	Color.a *= Alpha;
	if(Texture != nullptr)
		Graphics.DrawSprite(glm::vec3(DrawPosition, ObjectStat.RendersStat->Z), Texture, Color, 0.0f, glm::vec2(Scale));
}

// Draws a prop
void _EditorState::DrawProp(float OffsetX, float OffsetY, const _Prop *Prop, float Alpha) const {

	// Check if object is in view
	glm::vec2 DrawPosition(Prop->Position.x + OffsetX, Prop->Position.y + OffsetY);
	if(!Camera->IsCircleInView(DrawPosition, Prop->Stats.Radius))
		return;

	// Draw icon
	glm::vec4 Color(COLOR_WHITE);
	Color.a *= Alpha;
	Graphics.SetColor(Color);
	Prop->Render();
}

// Processes clicks on the buttons
void _EditorState::ProcessIcons(int Index, int Type) {

	switch(Index) {
	/*
		case ICON_LAYER1:
			ExecuteUpdateLayer(MAPLAYER_BASE, false);
		break;
		case ICON_LAYER2:
			ExecuteUpdateLayer(MAPLAYER_FLOOR0, false);
		break;
		case ICON_WALL:
			ExecuteUpdateLayer(MAPLAYER_WALL, false);
		break;
		case ICON_FORE:
			ExecuteUpdateLayer(MAPLAYER_FORE, false);
		break;
		*/
		case ICON_TILES:
			ExecuteSwitchMode(EDITMODE_TILES);
		break;
		case ICON_BLOCK:
			ExecuteSwitchMode(EDITMODE_BLOCKS);
		break;
		case ICON_OBJECTS:
			ExecuteSwitchMode(EDITMODE_OBJECTS);
		break;
		case ICON_PROPS:
			ExecuteSwitchMode(EDITMODE_PROPS);
		break;
		case ICON_NONE:
			ExecuteDeselect();
		break;
		case ICON_DELETE:
			ExecuteDelete();
		break;
		case ICON_COPY:
			ExecuteCopy();
		break;
		case ICON_PASTE:
			ExecutePaste(false);
		break;
		case ICON_SHOW:
			ExecuteHighlightBlocks();
		break;
		case ICON_CLEAR:
			ExecuteClear();
		break;
		case ICON_GRID:
			if(IsShiftDown)
				ExecuteUpdateGridMode(-1);
			else
				ExecuteUpdateGridMode(1);
		break;
		case ICON_LOAD:
			ExecuteIOCommand(EDITINPUT_LOAD);
		break;
		case ICON_SAVE:
			ExecuteIOCommand(EDITINPUT_SAVE);
		break;
		case ICON_TEST:
			ExecuteTest();
		break;
	}
}

// Processes clicks on the block buttons
void _EditorState::ProcessBlockIcons(int Index, int Type) {

	switch(Index) {
		case ICON_WALK:
			ExecuteWalkable();
		break;
		case ICON_RAISE:
			ExecuteChangeZ(0.5f, !IsShiftDown);
		break;
		case ICON_LOWER:
			ExecuteChangeZ(-0.5f, !IsShiftDown);
		break;
	}
}

// Adds an object to the list
void _EditorState::SpawnObject(const glm::vec2 &Position, const std::string &Identifier, bool Align) {
	glm::vec2 SpawnPosition;

	if(Align)
		SpawnPosition = AlignToGrid(Position);
	else
		SpawnPosition = Position;

	Map->ObjectSpawns.push_back(new _Spawn(Identifier, SpawnPosition));
}

// Add prop
void _EditorState::SpawnProp(const glm::vec2 &Position, const std::string &Identifier, bool Align) {
	glm::vec2 SpawnPosition;

	if(Align)
		SpawnPosition = AlignToGrid(Position);
	else
		SpawnPosition = Position;

	_Prop *Prop = Stats->CreateProp(Identifier);
	if(!Prop)
		return;

	Prop->Position = glm::vec3(WorldCursor, 0.0f);
	Map->AddProp(Prop);
}

// Determines if an object is part of the selected objects list
bool _EditorState::ObjectInSelectedList(_Spawn *Object) {

	for(auto Iterator : SelectedObjects) {
		if(Object == Iterator)
			return true;
	}

	return false;
}

// Executes the walkable command
void _EditorState::ExecuteWalkable() {
	if(BlockSelected())
		SelectedBlock->Collision = !SelectedBlock->Collision;
	else
		Collision = !Collision;
}

// Executes the change z command
void _EditorState::ExecuteChangeZ(float Change, int Type) {
	if(Type == 0) {
		if(BlockSelected())
			SelectedBlock->Start.z += Change;
		else
			DrawStart.z += Change;
	}
	else {
		if(BlockSelected())
			SelectedBlock->End.z += Change;
		else
			DrawEnd.z += Change;
	}
}

// Executes the change checkpoint command
void _EditorState::ExecuteUpdateCheckpointIndex(int Value) {
	CheckpointIndex += Value;

	if(CheckpointIndex < 0)
		CheckpointIndex = 0;
}

// Executes the an I/O command
void _EditorState::ExecuteIOCommand(int Type) {
	EditorInput = Type;
	InputBox->Focused = true;
	_Label *Label = (_Label *)InputBox->GetChildren()[0];
	Label->Text = InputBoxStrings[Type];
	InputBox->Text = SavedText[Type];
}

// Executes the clear map command
void _EditorState::ExecuteClear() {
	LoadMap("", false);
	SavedText[EDITINPUT_SAVE] = "";
}

// Executes the test command
void _EditorState::ExecuteTest() {

	// TODO catch exception
	Map->Save(EDITOR_TESTLEVEL);

	ExecuteDeselect();
	ClearClipboard();

	ClientState.SetTestMode(true);
	ClientState.SetFromEditor(true);
	ClientState.SetLevel(EDITOR_TESTLEVEL);
	ClientState.SetCheckpointIndex(CheckpointIndex);
	Framework.ChangeState(&ClientState);
}

// Executes the delete command
void _EditorState::ExecuteDelete() {

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(BlockSelected()) {
				//Map->RemoveBlock(CurrentLayer, SelectedBlockIndex);
				DeselectBlock();
			}
		break;
		default:
			if(ObjectsSelected()) {
				Map->RemoveObjectSpawns(SelectedObjectIndices);
				DeselectObjects();
				ClipboardObjects.clear();
			}
		break;
	}
}

// Executes the copy command
void _EditorState::ExecuteCopy() {

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(BlockSelected()) {
				ClipboardBlock = *SelectedBlock;
				DeselectBlock();
				BlockCopied = true;
			}
		break;
		default:
			if(ObjectsSelected()) {
				CopiedPosition = WorldCursor;
				ClipboardObjects = SelectedObjects;
			}
		break;
	}
}

// Executes the paste command
void _EditorState::ExecutePaste(bool Viewport) {
	glm::vec2 StartPosition;

	if(Viewport)
		StartPosition = WorldCursor;
	else
		StartPosition = Camera->GetPosition();

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(BlockCopied) {
				int Width = ClipboardBlock.End.x - ClipboardBlock.Start.x;
				int Height = ClipboardBlock.End.y - ClipboardBlock.Start.y;
				ClipboardBlock.Start = glm::vec3(Map->GetValidPosition(glm::vec2(StartPosition)), ClipboardBlock.Start.z);
				ClipboardBlock.End = glm::vec3(Map->GetValidPosition(glm::vec2(StartPosition.x + Width, StartPosition.y + Height)), ClipboardBlock.End.z);

				Map->AddBlock(ClipboardBlock);
			}
		break;
		default:
			for(auto Iterator : ClipboardObjects) {
				SpawnObject(Map->GetValidPosition(StartPosition - CopiedPosition + Iterator->Position), Iterator->Identifier, IsShiftDown);
			}
		break;
	}
}

// Executes the deselect command
void _EditorState::ExecuteDeselect() {
	DeselectBlock();
	DeselectObjects();
}

// Executes the update selected palette command
void _EditorState::ExecuteUpdateSelectedPalette(int Change) {
	std::vector<_Element *> &Children = PaletteElement[CurrentPalette]->GetChildren();
	if(!Brush[CurrentPalette]) {
		Brush[CurrentPalette] = (_Button *)Children[0];
		return;
	}

	int CurrentIndex = Brush[CurrentPalette]->ID;
	CurrentIndex += Change;
	if(CurrentIndex >= (int)Children.size())
		CurrentIndex = 0;
	else if(CurrentIndex < 0)
		CurrentIndex = Children.size() - 1;

	ExecuteSelectPalette((_Button *)Children[CurrentIndex], 0);
}

// Executes the select palette command
void _EditorState::ExecuteSelectPalette(_Button *Button, int ClickType) {
	if(!Button)
		return;

	// Didn't click a button
	if(Button->UserData == 0)
		return;

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(!Button)
				return;

			if(BlockSelected()) {
				SelectedBlock->Texture = Button->Style->Texture;
			}
		break;
		default:
		break;
	}

	if(ClickType == 0)
		Brush[CurrentPalette] = Button;
}

// Executes the update grid command
void _EditorState::ExecuteUpdateGridMode(int Change) {
	GridMode += Change;
	if(GridMode > 10)
		GridMode = 0;
	else if(GridMode < 0)
		GridMode = 10;
}

// Executes the highlight command
void _EditorState::ExecuteHighlightBlocks() {
	HighlightBlocks = !HighlightBlocks;

	Assets.Buttons["button_editor_show"]->Enabled = HighlightBlocks;
}

// Executes the toggle editor mode
void _EditorState::ExecuteSwitchMode(int State) {

	// Toggle icons
	if(CurrentPalette != State) {
		ModeButtons[CurrentPalette]->Enabled = false;
		ModeButtons[State]->Enabled = true;

		// Set state
		CurrentPalette = State;
	}
}

// Executes the update block limit command
void _EditorState::ExecuteUpdateBlockLimits(int Direction, bool Expand) {
	glm::vec3 Start, End;
	bool Change = false;
	if(CurrentPalette == EDITMODE_BLOCKS && BlockSelected()) {
		Start = SelectedBlock->Start;
		End = SelectedBlock->End;
		Change = true;
	}

	if(Change) {
		switch(Direction) {
			case 0:
				if(Expand)
					Start.x--;
				else
					End.x--;
			break;
			case 1:
				if(Expand)
					Start.y--;
				else
					End.y--;
			break;
			case 2:
				if(Expand)
					End.x++;
				else
					Start.x++;
			break;
			case 3:
				if(Expand)
					End.y++;
				else
					Start.y++;
			break;
		}

		// Check limits
		if(Start.x > End.x)
			Start.x = End.x;

		if(Start.y > End.y)
			Start.y = End.y;

		if(End.x < Start.x)
			End.x = Start.x;

		if(End.y < Start.y)
			End.y = Start.y;

		if(CurrentPalette == EDITMODE_BLOCKS && BlockSelected()) {
			SelectedBlock->Start = glm::vec3(Map->GetValidCoord(glm::vec2(Start)), SelectedBlock->Start.z);
			SelectedBlock->End = glm::vec3(Map->GetValidCoord(glm::vec2(End)), SelectedBlock->End.z);
		}
	}
}

// Selects an object
void _EditorState::SelectObject() {
	ClickedPosition = WorldCursor;

	_Spawn *SelectedObject;
	size_t Index;
	Map->GetSelectedObject(WorldCursor, EDITOR_OBJECTRADIUS * EDITOR_OBJECTRADIUS, &SelectedObject, &Index);
	if(SelectedObject != nullptr) {
		IsMoving = true;

		// Single object selected
		if(!ObjectInSelectedList(SelectedObject)) {
			DeselectObjects();
			SelectedObjects.push_back(SelectedObject);
			SelectedObjectIndices.push_back(Index);
		}
	}
	else {
		DeselectObjects();
		DraggingBox = true;
	}
}

// Selects objects
void _EditorState::SelectObjects() {
	DeselectObjects();
	Map->GetSelectedObjects(ClickedPosition, WorldCursor, &SelectedObjects, &SelectedObjectIndices);
}

// Aligns an object to the grid
glm::vec2 _EditorState::AlignToGrid(const glm::vec2 &Position) const {

	return glm::vec2(glm::ivec2(Position * AlignDivisor)) / AlignDivisor;
}

// Get tentative position
glm::vec2 _EditorState::GetMoveDeltaPosition(const glm::vec2 &Position) {
	glm::vec2 NewPosition;
	if(IsShiftDown)
		NewPosition = AlignToGrid(Map->GetValidPosition(Position + MoveDelta));
	else
		NewPosition = Map->GetValidPosition(Position + MoveDelta);

	return NewPosition;
}

// Clears all the objects in the clipboard
void _EditorState::ClearClipboard() {
	BlockCopied = false;
	ClipboardObjects.clear();
}

void _EditorState::DeselectObjects() {
	SelectedObjects.clear();
	SelectedObjectIndices.clear();
}
