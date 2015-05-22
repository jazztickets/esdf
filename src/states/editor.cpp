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
#include <objects/physics.h>
#include <objects/render.h>
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
#include <grid.h>
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

// Set up ui callbacks
static std::unordered_map<std::string, _EditorState::CallbackType> IconCallbacks = {
	{ "button_editor_mode_tiles",   &_EditorState::ExecuteSwitchMode },
	{ "button_editor_mode_block",   &_EditorState::ExecuteSwitchMode },
	{ "button_editor_mode_objects", &_EditorState::ExecuteSwitchMode },
	{ "button_editor_mode_props",   &_EditorState::ExecuteSwitchMode },
	{ "button_editor_walk",         &_EditorState::ExecuteWalkable },
	{ "button_editor_lower",        &_EditorState::ExecuteChangeZ },
	{ "button_editor_raise",        &_EditorState::ExecuteChangeZ },
	{ "button_editor_deselect",     &_EditorState::ExecuteDeselect },
	{ "button_editor_delete",       &_EditorState::ExecuteDelete },
	{ "button_editor_copy",         &_EditorState::ExecuteCopy },
	{ "button_editor_paste",        &_EditorState::ExecutePaste },
	{ "button_editor_show",         &_EditorState::ExecuteHighlightBlocks },
	{ "button_editor_new",          &_EditorState::ExecuteNew },
	{ "button_editor_grid",         &_EditorState::ExecuteUpdateGridMode },
	{ "button_editor_load",         &_EditorState::ExecuteIOCommand },
	{ "button_editor_save",         &_EditorState::ExecuteIOCommand },
	{ "button_editor_test",         &_EditorState::ExecuteTest },
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
	GridVertices = nullptr;

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

	// Enable last palette
	if(SavedPalette != -1) {
		CurrentPalette = SavedPalette;
		ModeButtons[0]->Enabled = false;
		ModeButtons[SavedPalette]->Enabled = true;
	}
}

void _EditorState::Close() {
	SavedCameraPosition = Camera->GetPosition();
	SavedPalette = CurrentPalette;

	for(int i = 0; i < EDITMODE_COUNT; i++)
		ClearPalette(i);

	delete Camera;
	delete Map;
	delete[] GridVertices;

	Camera = nullptr;
	Map = nullptr;
}

// Load a level
bool _EditorState::LoadMap(const std::string &File, bool UseSavedCameraPosition) {
	if(Map)
		delete Map;

	Map = new _Map(File, Stats);
	Map->SetCamera(Camera);

	// Allocate space for grid lines
	delete[] GridVertices;
	int Lines = int(Map->Grid->Size.y-1) + int(Map->Grid->Size.y-1);
	GridVertices = new float[Lines * 4];

	// Set up editor state
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
		Palette.push_back(_Palette(std::to_string(i), std::to_string(i), nullptr, nullptr, Map->TileAtlas, i, COLOR_WHITE));
	}

	LoadPaletteButtons(Palette, EDITMODE_TILES);

	return true;
}

void _EditorState::ResetState() {
	WorldCursor = glm::vec2(0);
	SelectedBlock = nullptr;
	EditorInputType = -1;
	CheckpointIndex = 0;
	ClickedPosition = glm::vec2(0);
	CopiedPosition = glm::vec2(0);
	CurrentPalette = EDITMODE_TILES;
	GridMode = EDITOR_DEFAULT_GRIDMODE;
	Collision = 0;
	HighlightBlocks = false;
	SelectedObjects.clear();
	ClipboardObjects.clear();
	Assets.Buttons["button_editor_show"]->Enabled = false;

	AlignDivisor = EDITOR_ALIGN_DIVISOR;
	IsShiftDown = false;
	IsCtrlDown = false;
	DraggingBox = false;
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
	if(IsDrawing || !KeyEvent.Pressed)
		return;

	// See if the user is entering in text
	if(EditorInputType != -1) {
		switch(KeyEvent.Key) {
			case SDL_SCANCODE_RETURN: {
				const std::string InputText = InputBox->Text;
				switch(EditorInputType) {
					case EDITINPUT_LOAD: {
						if(InputText == "")
							break;

						if(LoadMap(InputText, false))
							SavedText[EDITINPUT_SAVE] = InputText;

					} break;
					case EDITINPUT_SAVE:
						if(InputText == "" || !Map->Save(InputText))
							SavedText[EditorInputType] = "";
						else {
							SavedText[EditorInputType] = InputText;
						}

						ExecuteDeselect(this, nullptr);
					break;
				}
				EditorInputType = -1;
			} break;
			case SDL_SCANCODE_ESCAPE:
				EditorInputType = -1;
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
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_tiles"]);
			break;
			case SDL_SCANCODE_2:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_block"]);
			break;
			case SDL_SCANCODE_3:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_objects"]);
			break;
			case SDL_SCANCODE_4:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_props"]);
			break;
			case SDL_SCANCODE_GRAVE:
				ExecuteDeselect(this, nullptr);
			break;
			case SDL_SCANCODE_D:
				ExecuteDelete(this, nullptr);
			break;
			case SDL_SCANCODE_C:
				ExecuteCopy(this, nullptr);
			break;
			case SDL_SCANCODE_V:
				ExecutePaste(this, nullptr);
			break;
			case SDL_SCANCODE_G:
				//ExecuteUpdateGridMode(this, nullptr);
				if(IsMoving)
					CancelMove();
				else if(SelectedObjects.size()) {
					IsMoving = true;
					ClickedPosition = WorldCursor;
				}
			break;
			case SDL_SCANCODE_B:
				ExecuteHighlightBlocks(this, Assets.Buttons["button_editor_show"]);
			break;
			case SDL_SCANCODE_A:
				ExecuteWalkable(this, nullptr);
			break;
			case SDL_SCANCODE_KP_MINUS:
				ExecuteChangeZ(this, Assets.Buttons["button_editor_lower"]);
			break;
			case SDL_SCANCODE_KP_PLUS:
				ExecuteChangeZ(this, Assets.Buttons["button_editor_raise"]);
			break;
			case SDL_SCANCODE_N:
				if(IsCtrlDown)
					ExecuteNew(this, nullptr);
			break;
			case SDL_SCANCODE_L:
				ExecuteIOCommand(this, Assets.Buttons["button_editor_load"]);
				BlockTextEvent = true;
			break;
			case SDL_SCANCODE_S:
				ExecuteIOCommand(this, Assets.Buttons["button_editor_save"]);
				BlockTextEvent = true;
			break;
			case SDL_SCANCODE_T:
				ExecuteTest(this, nullptr);
			break;
		}
	}
}

// Text event handler
void _EditorState::TextEvent(const char *Text) {
	if(EditorInputType != -1) {
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
	if(Clicked && (intptr_t)Clicked->UserData != -1) {
		if(IconCallbacks.find(Clicked->Identifier) != IconCallbacks.end())
			IconCallbacks[Clicked->Identifier](this, Clicked);
	}

	if(CurrentPalette == EDITMODE_BLOCKS) {
		_Element *Clicked = BlockElement->GetClickedElement();
		if(Clicked && (intptr_t)Clicked->UserData != -1) {
			if(IconCallbacks.find(Clicked->Identifier) != IconCallbacks.end())
				IconCallbacks[Clicked->Identifier](this, Clicked);
		}
	}

	// Handle viewport clicks
	if(Input.GetMouse().x < Graphics.ViewportSize.x && Input.GetMouse().y < Graphics.ViewportSize.y) {
		if(MouseEvent.Pressed) {
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
							case EDITMODE_PROPS:
								if(Button) {

									glm::vec2 Position = WorldCursor;
									if(IsShiftDown)
										Position = AlignToGrid(WorldCursor);

									_Object *Object = Stats->CreateObject(Button->Identifier, false);
									Object->Map = Map;
									Object->Physics->ForcePosition(Position);
									Map->AddObject(Object);
									Map->Grid->AddObject(Object);
								}
							break;
						}
					}

					if(IsMoving)
						ConfirmMove();
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
								SelectedBlock = Map->GetSelectedBlock(WorldCursor);
								if(BlockSelected()) {
									OldStart = SelectedBlock->Start;
									OldEnd = SelectedBlock->End;
									SavedIndex = WorldCursor;
									IsMoving = true;
								}
							break;
							case EDITMODE_OBJECTS:
							case EDITMODE_PROPS:
								ClickedPosition = WorldCursor;
								DraggingBox = true;
							break;
							default:
							break;
						}
					}
				break;
			}
		}
	}
	// Handle ui clicks
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
					for(auto Object : SelectedObjects)
						Object->Physics->NetworkPosition = Object->Physics->Position;
				}

				if(DraggingBox) {
					DraggingBox = false;
					DeselectObjects();
					glm::vec4 AABB;
					AABB[0] = std::min(ClickedPosition.x, WorldCursor.x);
					AABB[1] = std::min(ClickedPosition.y, WorldCursor.y);
					AABB[2] = std::max(ClickedPosition.x, WorldCursor.x);
					AABB[3] = std::max(ClickedPosition.y, WorldCursor.y);
					Map->GetSelectedObjects(AABB, &SelectedObjects);

					// Save original position
					for(auto Object : SelectedObjects)
						Object->Physics->NetworkPosition = Object->Physics->Position;
				}

				if(SelectedBlock) {
					Map->Grid->RemoveBlock(SelectedBlock);
					SelectedBlock->Start = DrawStart;
					SelectedBlock->End = DrawEnd;
					Map->Grid->AddBlock(SelectedBlock);
				}
			break;
		}
	}
}

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

void _EditorState::WindowEvent(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(Graphics.AspectRatio);
}

// Update
void _EditorState::Update(double FrameTime) {

	CommandElement->Update(FrameTime, Input.GetMouse());
	BlockElement->Update(FrameTime, Input.GetMouse());
	PaletteElement[CurrentPalette]->Update(FrameTime, Input.GetMouse());
	if(EditorInputType != -1) {
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
	Map->Update(FrameTime, 0);

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
		else if(Offset.x + OldEnd.x >= Map->Grid->Size.x)
			Offset.x = Map->Grid->Size.x - OldEnd.x;

		// Check y bounds
		if(Offset.y + OldStart.y < 0)
			Offset.y = -OldStart.y;
		else if(Offset.y + OldEnd.y >= Map->Grid->Size.y)
			Offset.y = Map->Grid->Size.y - OldEnd.y;

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
					glm::ivec2 TilePosition = Map->Grid->GetValidCoord(WorldCursor);
					Map->Grid->Tiles[TilePosition.x][TilePosition.y].TextureIndex = Brush[EDITMODE_TILES]->TextureIndex;
				}

				if(FinishedDrawing)
					FinishedDrawing = IsDrawing = false;
			}
		break;
		case EDITMODE_BLOCKS:

			// Finish drawing a block and add it to the list
			if(FinishedDrawing) {
				if(Brush[EDITMODE_BLOCKS]) {
					_Block *Block = new _Block();
					Block->Start = DrawStart;
					Block->End = DrawEnd;
					Block->Texture = Brush[EDITMODE_BLOCKS]->Style->Texture;

					Map->AddBlock(Block);
				}

				FinishedDrawing = IsDrawing = false;
			}
		break;
		default:
			if(IsMoving) {
				for(auto Object : SelectedObjects) {
					if(!Object->Physics)
						continue;

					Object->Physics->Position = Map->GetValidPosition(Object->Physics->NetworkPosition + WorldCursor - ClickedPosition);
				}
			}
		break;
	}
}

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
	_Block *ExceptionBlock = nullptr;
	if(SelectedBlock && IsMoving)
		ExceptionBlock = SelectedBlock;

	Map->RenderWalls(ExceptionBlock);

	// Draw objects
	Map->RenderObjects(BlendFactor);

	// Outline selected objects
	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetColor(COLOR_WHITE);
	Graphics.SetVBO(VBO_CIRCLE);
	Graphics.SetDepthTest(false);
	for(auto Object : SelectedObjects) {
		if(!Object->Physics)
			continue;
		Graphics.DrawCircle(glm::vec3(Object->Physics->Position, ITEM_Z + 0.05f), EDITOR_OBJECTRADIUS);
	}
	Graphics.SetDepthTest(true);

	// Draw tentative asset
	switch(CurrentPalette) {
		case EDITMODE_TILES:
		break;
		case EDITMODE_BLOCKS:
			Graphics.SetColor(COLOR_WHITE);
			if(IsDrawing && Brush[CurrentPalette]) {
				Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
				Graphics.SetVBO(VBO_CUBE);
				Graphics.DrawCube(glm::vec3(DrawStart), glm::vec3(DrawEnd - DrawStart), Brush[CurrentPalette]->Style->Texture);
			}
			else if(IsMoving) {
				Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
				Graphics.SetVBO(VBO_CUBE);
				Graphics.DrawCube(glm::vec3(DrawStart), glm::vec3(DrawEnd - DrawStart), SelectedBlock->Texture);
			}
		break;
		case EDITMODE_OBJECTS:
		case EDITMODE_PROPS:
			if(Brush[CurrentPalette]) {

				_Object *Object = (_Object *)Brush[CurrentPalette]->UserData;
				if(!Object)
					break;

				// Check if object is in view
				glm::vec2 DrawPosition(WorldCursor.x, WorldCursor.y);
				if(!Camera->IsCircleInView(DrawPosition, Object->Render->Stat.Scale))
					break;

				// Create temp object
				Object->Physics->ForcePosition(DrawPosition);

				// Draw
				glm::vec4 Color(COLOR_WHITE);
				Color.a *= 0.5f;
				Graphics.SetColor(Color);
				Object->Render->Draw3D(BlendFactor);
			}
		break;
	}

	Graphics.SetDepthTest(false);
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetVBO(VBO_NONE);

	// Draw map boundaries
	Graphics.SetColor(COLOR_RED);
	Graphics.DrawRectangle(glm::vec2(-0.01f, -0.01f), glm::vec2(Map->Grid->Size.x + 0.01f, Map->Grid->Size.y + 0.01f));

	// Draw grid
	Map->RenderGrid(GridMode, GridVertices);

	// Outline the blocks
	if(HighlightBlocks)
		Map->HighlightBlocks();

	// Outline selected block
	Graphics.SetColor(COLOR_WHITE);
	if(BlockSelected())
		Graphics.DrawRectangle(glm::vec2(DrawStart), glm::vec2(DrawEnd));

	// Dragging a box around object
	if(DraggingBox)
		Graphics.DrawRectangle(ClickedPosition, WorldCursor);

	// Draw a block
	Graphics.SetColor(COLOR_GREEN);
	if(IsDrawing && CurrentPalette == EDITMODE_BLOCKS)
		Graphics.DrawRectangle(glm::vec2(DrawStart), glm::vec2(DrawEnd));

	// Setup 2D transformation
	Graphics.Setup2D();

	/*
	glm::ivec2 Start(Camera->GetAABB()[0], Camera->GetAABB()[1]);
	glm::ivec2 End(Camera->GetAABB()[2], Camera->GetAABB()[3]);
	for(int X = Start.x; X < End.x; X++) {
		for(int Y = Start.y; Y < End.y; Y++) {
			if(X >= 0 && Y >= 0 && X < Map->Size.x && Y < Map->Size.y) {
				glm::ivec2 P;
				Camera->ConvertWorldToScreen(glm::vec2(X+0.5f, Y+0.5f), P);
				std::ostringstream Buffer;
				Buffer << Map->GetTiles()[X][Y].Blocks.size();
				Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), P);
				Buffer.str("");
			}
		}
	}
	*/

	// Draw viewport outline
	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.SetColor(COLOR_DARK);
	Graphics.DrawRectangle(glm::vec2(0, 0), glm::vec2(Graphics.ViewportSize.x, Graphics.ViewportSize.y));

	// Draw text
	if(EditorInputType != -1) {
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
}

// Load palette buttons
void _EditorState::LoadPalettes() {

	// Load block textures
	{
		std::vector<_Palette> Palette;
		_Files Files(TEXTURES_PATH + TEXTURES_BLOCKS);
		for(const auto &File : Files.Nodes) {
			std::string Identifier = TEXTURES_BLOCKS + File;
			Palette.push_back(_Palette(Identifier, Identifier, nullptr, Assets.Textures[Identifier], nullptr, 0, COLOR_WHITE));
		}

		LoadPaletteButtons(Palette, EDITMODE_BLOCKS);
	}

	{
		// Load objects
		std::vector<_Palette> Palette;
		std::vector<_Palette> PaletteProps;
		for(auto Iterator : Stats->Objects) {
			if(Iterator.second.RenderStat) {
				const _ObjectStat &ObjectStat = Iterator.second;

				// Check for a render/physics component
				if(!ObjectStat.RenderStat || !ObjectStat.PhysicsStat)
					break;

				// Create object
				_Object *Object = new _Object();

				// Add components
				_Render *Render = new _Render(Object, *ObjectStat.RenderStat);
				_Physics *Physics = new _Physics(Object);
				Object->Render = Render;
				Object->Physics = Physics;
				Object->Render->Program = Assets.Programs[ObjectStat.RenderStat->ProgramIdentifier];
				Object->Render->Texture = Assets.Textures[ObjectStat.RenderStat->TextureIdentifier];
				Object->Render->Mesh = Assets.Meshes[ObjectStat.RenderStat->MeshIdentifier];

				if(!Object->Render->Mesh)
					Palette.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Name, Object, Object->Render->Texture, nullptr, 0, COLOR_WHITE));
				else
					PaletteProps.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Identifier, Object, Object->Render->Texture, nullptr, 0, COLOR_WHITE));
			}
		}

		LoadPaletteButtons(Palette, EDITMODE_OBJECTS);
		LoadPaletteButtons(PaletteProps, EDITMODE_PROPS);
	}
}

// Free memory used by palette
void _EditorState::ClearPalette(int Type) {
	std::vector<_Element *> &Children = PaletteElement[Type]->Children;
	for(size_t i = 0; i < Children.size(); i++) {
		delete (_Object *)(Children[i]->UserData);
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
		Button->UserData = Palette[i].UserData;
		PaletteElement[Type]->Children.push_back(Button);

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
				if(IconTexture)
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
				auto Object = *SelectedObjects.begin();
				IconIdentifier = Object->Identifier;
				IconText = Object->Name;
				if(Object->Render)
					IconTexture = Object->Render->Texture;
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

	Graphics.SetColor(IconColor);
	if(IconTexture)
		Graphics.DrawImage(Bounds, IconTexture);
	else if(IconAtlas)
		Graphics.DrawAtlas(Bounds, IconAtlas->Texture, IconAtlas->GetTextureCoords(IconTextureIndex));
}

// Executes the toggle editor mode
void _EditorState::ExecuteSwitchMode(_EditorState *State, _Element *Element) {
	int Palette = (intptr_t)Element->UserData;

	// Toggle icons
	if(State->CurrentPalette != Palette) {
		State->ModeButtons[State->CurrentPalette]->Enabled = false;
		State->ModeButtons[Palette]->Enabled = true;

		// Set state
		State->CurrentPalette = Palette;
		State->DeselectBlock();
	}
}

// Executes the walkable command
void _EditorState::ExecuteWalkable(_EditorState *State, _Element *Element) {
	//if(State->BlockSelected())
	//	State->SelectedBlock->Collision = !State->SelectedBlock->Collision;
	//else
	//	State->Collision = !State->Collision;
}

// Executes the change z command
void _EditorState::ExecuteChangeZ(_EditorState *State, _Element *Element) {
	float Change = -0.5f;
	if((intptr_t)Element->UserData)
		Change *= -1;

	// Type 0 = MinZ, Type 1 = MaxZ
	int Type = 1;
	if(State->IsShiftDown)
		Type = !Type;

	if(Type == 0) {
		if(State->BlockSelected())
			State->SelectedBlock->Start.z += Change;
		else
			State->DrawStart.z += Change;
	}
	else {
		if(State->BlockSelected())
			State->SelectedBlock->End.z += Change;
		else
			State->DrawEnd.z += Change;
	}
}

// Executes the deselect command
void _EditorState::ExecuteDeselect(_EditorState *State, _Element *Element) {
	State->DeselectBlock();
	State->DeselectObjects();
}

// Executes the delete command
void _EditorState::ExecuteDelete(_EditorState *State, _Element *Element) {

	switch(State->CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(State->BlockSelected()) {
				State->Map->RemoveBlock(State->SelectedBlock);
				State->DeselectBlock();
			}
		break;
		default:
			if(State->ObjectsSelected()) {
				for(auto Object : State->SelectedObjects)
					Object->Deleted = true;

				State->DeselectObjects();
				State->ClipboardObjects.clear();
			}
		break;
	}
}

// Executes the copy command
void _EditorState::ExecuteCopy(_EditorState *State, _Element *Element) {

	switch(State->CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(State->BlockSelected()) {
				//State->ClipboardBlock = *State->SelectedBlock;
				State->DeselectBlock();
			}
		break;
		default:
			if(State->ObjectsSelected()) {
				State->CopiedPosition = State->WorldCursor;
				State->ClipboardObjects = State->SelectedObjects;
			}
		break;
	}
}

// Executes the paste command
void _EditorState::ExecutePaste(_EditorState *State, _Element *Element) {
	glm::vec2 StartPosition;

	if(Element)
		StartPosition = State->Camera->GetPosition();
	else
		StartPosition = State->WorldCursor;

	switch(State->CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(State->ClipboardBlocks.size()) {
				//int Width = State->ClipboardBlock.End.x - State->ClipboardBlock.Start.x;
				//int Height = State->ClipboardBlock.End.y - State->ClipboardBlock.Start.y;
				//State->ClipboardBlock.Start = glm::vec3(State->Map->GetValidPosition(glm::vec2(StartPosition)), State->ClipboardBlock.Start.z);
				//State->ClipboardBlock.End = glm::vec3(State->Map->GetValidPosition(glm::vec2(StartPosition.x + Width, StartPosition.y + Height)), State->ClipboardBlock.End.z);

				//State->Map->AddBlock(State->ClipboardBlock);
			}
		break;
		default:
			for(auto Iterator : State->ClipboardObjects) {
				_Object *Object = State->Stats->CreateObject(Iterator->Identifier, false);
				Object->Map = State->Map;
				Object->Physics->ForcePosition(State->Map->GetValidPosition(StartPosition - State->CopiedPosition + Iterator->Physics->Position));
				State->Map->AddObject(Object);
				State->Map->Grid->AddObject(Object);
			}
		break;
	}
}

// Executes the highlight command
void _EditorState::ExecuteHighlightBlocks(_EditorState *State, _Element *Element) {
	State->HighlightBlocks = !State->HighlightBlocks;

	// Toggle button state
	((_Button *)Element)->Enabled = State->HighlightBlocks;
}

// Executes the clear map command
void _EditorState::ExecuteNew(_EditorState *State, _Element *Element) {
	State->LoadMap("", false);
	State->SavedText[EDITINPUT_SAVE] = "";
}

// Executes the an I/O command
void _EditorState::ExecuteIOCommand(_EditorState *State, _Element *Element) {

	// Get io type
	int Type = (intptr_t)Element->UserData;

	State->EditorInputType = Type;
	State->InputBox->Focused = true;
	_Label *Label = (_Label *)State->InputBox->Children[0];
	Label->Text = InputBoxStrings[Type];
	State->InputBox->Text = State->SavedText[Type];
}

// Executes the test command
void _EditorState::ExecuteTest(_EditorState *State, _Element *Element) {
	State->Map->Save(EDITOR_TESTLEVEL);

	ClientState.SetTestMode(true);
	ClientState.SetFromEditor(true);
	ClientState.SetLevel(EDITOR_TESTLEVEL);
	ClientState.SetCheckpointIndex(State->CheckpointIndex);
	Framework.ChangeState(&ClientState);
}

// Executes the update grid command
void _EditorState::ExecuteUpdateGridMode(_EditorState *State, _Element *Element) {
	int Change = 1;
	if(State->IsShiftDown)
		Change = -1;

	State->GridMode += Change;
	if(State->GridMode > 10)
		State->GridMode = 0;
	else if(State->GridMode < 0)
		State->GridMode = 10;
}

// Executes the change checkpoint command
void _EditorState::ExecuteUpdateCheckpointIndex(int Value) {
	CheckpointIndex += Value;

	if(CheckpointIndex < 0)
		CheckpointIndex = 0;
}

// Executes the select palette command
void _EditorState::ExecuteSelectPalette(_Button *Button, int ClickType) {
	if(!Button)
		return;

	// Didn't click a button
	if(Button->Style == 0)
		return;

	switch(CurrentPalette) {
		case EDITMODE_BLOCKS:
			if(!Button)
				return;

			if(BlockSelected()) {
				SelectedBlock->Texture = Button->Style->Texture;
			}
		break;
		case EDITMODE_OBJECTS:
		case EDITMODE_PROPS:

		break;
		default:
		break;
	}

	if(ClickType == 0)
		Brush[CurrentPalette] = Button;
}

// Aligns an object to the grid
glm::vec2 _EditorState::AlignToGrid(const glm::vec2 &Position) const {

	return glm::vec2(glm::ivec2(Position * AlignDivisor)) / AlignDivisor;
}

// Confirm a move operation
void _EditorState::ConfirmMove() {
	for(auto Object : SelectedObjects) {
		Object->Physics->NetworkPosition = Object->Physics->Position;
	}

	IsMoving = false;
}

// Cancel a move operation
void _EditorState::CancelMove() {
	for(auto Object : SelectedObjects) {
		Object->Physics->Position = Object->Physics->NetworkPosition;
	}

	IsMoving = false;
}
