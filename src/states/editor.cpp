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
#include <objects/shape.h>
#include <objects/zone.h>
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
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <SDL_keycode.h>
#include <SDL_mouse.h>

#include <program.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

_EditorState EditorState;

// Input box
const char *InputBoxStrings[EDITINPUT_COUNT] = {
	"Load map",
	"Save map",
	"Script function",
};

// Set up ui callbacks
static std::unordered_map<std::string, _EditorState::CallbackType> IconCallbacks = {
	{ "button_editor_mode_tile",    &_EditorState::ExecuteSwitchMode },
	{ "button_editor_mode_block",   &_EditorState::ExecuteSwitchMode },
	{ "button_editor_mode_object",  &_EditorState::ExecuteSwitchMode },
	{ "button_editor_mode_prop",    &_EditorState::ExecuteSwitchMode },
	{ "button_editor_mode_zone",    &_EditorState::ExecuteSwitchMode },
	{ "button_editor_walk",         &_EditorState::ExecuteWalkable },
	{ "button_editor_lower",        &_EditorState::ExecuteChangeZ },
	{ "button_editor_raise",        &_EditorState::ExecuteChangeZ },
	{ "button_editor_onenter",      &_EditorState::ExecuteIOCommand },
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
	ZoneElement = Assets.Elements["element_editor_zone"];
	InputBox = Assets.TextBoxes["textbox_editor_input"];

	// Create button groups
	PaletteElement[0] = Assets.Elements["element_editor_palette_tile"];
	PaletteElement[1] = Assets.Elements["element_editor_palette_block"];
	PaletteElement[2] = Assets.Elements["element_editor_palette_object"];
	PaletteElement[3] = Assets.Elements["element_editor_palette_prop"];
	PaletteElement[4] = Assets.Elements["element_editor_palette_zone"];

	// Assign palette buttons
	ModeButtons[0] = Assets.Buttons["button_editor_mode_tile"];
	ModeButtons[1] = Assets.Buttons["button_editor_mode_block"];
	ModeButtons[2] = Assets.Buttons["button_editor_mode_object"];
	ModeButtons[3] = Assets.Buttons["button_editor_mode_prop"];
	ModeButtons[4] = Assets.Buttons["button_editor_mode_zone"];

	// Reset state
	ResetState();
	GridVertices = nullptr;

	// Create camera
	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_EDITOR_DIVISOR);

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
		Camera->ForcePosition(glm::vec3(Map->GetStartingPositionByCheckpoint(0), CAMERA_DISTANCE));

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
	EditorInputType = -1;
	CheckpointIndex = 0;
	ClickedPosition = glm::vec2(0);
	CopiedPosition = glm::vec2(0);
	CurrentPalette = EDITMODE_TILES;
	GridMode = EDITOR_DEFAULT_GRIDMODE;
	Collision = 0;
	HighlightBlocks = false;
	TileBrushRadius = 0.5f;
	SelectedObjects.clear();
	ClipboardObjects.clear();
	Assets.Buttons["button_editor_show"]->Enabled = false;

	AlignDivisor = EDITOR_ALIGN_DIVISOR;
	IsShiftDown = false;
	IsCtrlDown = false;
	DraggingBox = false;
	IsDrawing = false;
	IsMoving = false;
	IgnoreTextEvent = false;

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

	for(int i = 0; i < EDITMODE_COUNT; i++) {
		ModeButtons[i]->Enabled = false;
		Brush[i] = nullptr;
	}

	// Load palettes
	LoadPalettes();
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
					case EDITINPUT_SCRIPT:
						for(auto &Object : SelectedObjects) {
							if(Object->Components.find("zone") != Object->Components.end()) {
								_Zone *Zone = (_Zone *)(Object->Components["zone"]);
								Zone->OnEnter = InputText;
							}
						}

						//SavedText[EditorInputType] = InputText;
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
			case SDL_SCANCODE_MINUS:
				ExecuteUpdateCheckpointIndex(-1);
			break;
			case SDL_SCANCODE_EQUALS:
				ExecuteUpdateCheckpointIndex(1);
			break;
			case SDL_SCANCODE_1:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_tile"]);
			break;
			case SDL_SCANCODE_2:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_block"]);
			break;
			case SDL_SCANCODE_3:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_object"]);
			break;
			case SDL_SCANCODE_4:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_prop"]);
			break;
			case SDL_SCANCODE_5:
				ExecuteSwitchMode(this, Assets.Buttons["button_editor_mode_zone"]);
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
			case SDL_SCANCODE_H:
				ExecuteUpdateGridMode(this, nullptr);
			break;
			case SDL_SCANCODE_G:
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
				if(CurrentPalette == EDITMODE_BLOCKS)
					ExecuteWalkable(this, nullptr);
			break;
			case SDL_SCANCODE_E:
				if(CurrentPalette == EDITMODE_ZONE) {
					ExecuteIOCommand(this, Assets.Buttons["button_editor_onenter"]);
					IgnoreTextEvent = true;
				}
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
				IgnoreTextEvent = true;
			break;
			case SDL_SCANCODE_S:
				ExecuteIOCommand(this, Assets.Buttons["button_editor_save"]);
				IgnoreTextEvent = true;
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
		if(IgnoreTextEvent)
			IgnoreTextEvent = false;
		else
			InputBox->HandleTextEvent(Text);
	}
}

// Mouse handler
void _EditorState::MouseEvent(const _MouseEvent &MouseEvent) {

	if(MouseEvent.Button == SDL_BUTTON_LEFT) {
		CommandElement->HandleInput(MouseEvent.Pressed);
		BlockElement->HandleInput(MouseEvent.Pressed);
		ZoneElement->HandleInput(MouseEvent.Pressed);
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
	else if(CurrentPalette == EDITMODE_ZONE) {
		_Element *Clicked = ZoneElement->GetClickedElement();
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
							break;
							case EDITMODE_BLOCKS:
							case EDITMODE_ZONE:
								SavedIndex = WorldCursor;
								IsDrawing = true;
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
					Camera->Set2DPosition(WorldCursor);
				break;
				// Select object
				case SDL_BUTTON_MIDDLE:
					if(!IsDrawing) {
						switch(CurrentPalette) {
							case EDITMODE_BLOCKS:
							case EDITMODE_OBJECTS:
							case EDITMODE_PROPS:
							case EDITMODE_ZONE:
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
					_Button *Button = Brush[CurrentPalette];

					switch(CurrentPalette) {
						case EDITMODE_BLOCKS:
							if(Button) {
								_Object *Object = Stats->CreateObject(Button->Identifier, false);
								Object->Map = Map;
								Object->Render->Texture = Button->Style->Texture;
								Object->Physics->LastPosition = Object->Physics->Position = (DrawStart + DrawEnd) / 2.0f;
								Object->Shape->HalfWidth = (DrawEnd - DrawStart) / 2.0f;
								Map->AddObject(Object);
								Map->Grid->AddObject(Object);
							}
						break;
						case EDITMODE_ZONE:
							_Object *Object = Stats->CreateObject(Button->Identifier, false);
							Object->Map = Map;
							Object->Physics->LastPosition = Object->Physics->Position = (DrawStart + DrawEnd) / 2.0f;
							Object->Shape->HalfWidth = (DrawEnd - DrawStart) / 2.0f;
							Object->Shape->HalfWidth.z = 0.0f;
							Object->Physics->LastPosition.z = Object->Physics->Position.z = 0.0f;
							Map->AddObject(Object);
							Map->Grid->AddObject(Object);
						break;
					}

					IsDrawing = false;
				}
			break;
			case SDL_BUTTON_MIDDLE:
				if(DraggingBox) {
					DraggingBox = false;

					glm::vec4 AABB;
					AABB[0] = std::min(ClickedPosition.x, WorldCursor.x) + MAP_BLOCK_ADJUST;
					AABB[1] = std::min(ClickedPosition.y, WorldCursor.y) + MAP_BLOCK_ADJUST;
					AABB[2] = std::max(ClickedPosition.x, WorldCursor.x) - MAP_BLOCK_ADJUST;
					AABB[3] = std::max(ClickedPosition.y, WorldCursor.y) - MAP_BLOCK_ADJUST;

					std::list<_Object *> Selection;
					Map->GetSelectedObjects(AABB, &Selection);

					// Filter selection by palette type
					SelectedObjects.clear();
					for(auto &Object : Selection) {
						switch(CurrentPalette) {
							case EDITMODE_BLOCKS:
								if(Object->Render->Stats->Layer == Assets.Layers["block"].Layer)
									SelectedObjects.push_back(Object);
							break;
							case EDITMODE_OBJECTS:
								if(Object->Render->Stats->Layer != Assets.Layers["block"].Layer && Object->Render->Stats->Layer != Assets.Layers["zone"].Layer && !Object->Render->Mesh)
									SelectedObjects.push_back(Object);
							break;
							case EDITMODE_PROPS:
								if(Object->Render->Stats->Layer != Assets.Layers["block"].Layer && Object->Render->Stats->Layer != Assets.Layers["zone"].Layer && Object->Render->Mesh)
									SelectedObjects.push_back(Object);
							break;
							case EDITMODE_ZONE:
								if(Object->Render->Stats->Layer == Assets.Layers["zone"].Layer)
									SelectedObjects.push_back(Object);
							break;
							default:
							break;
						}
					}

					// Save original position
					for(auto &Object : SelectedObjects)
						Object->Physics->NetworkPosition = Object->Physics->Position;
				}
			break;
		}
	}
}

// Mouse wheel handler
void _EditorState::MouseWheelEvent(int Direction) {

	if(Input.GetMouse().x < Graphics.ViewportSize.x && Input.GetMouse().y < Graphics.ViewportSize.y) {
		if(IsCtrlDown) {
			TileBrushRadius += 1.0f * Direction;
			if(TileBrushRadius < 0.5f)
				TileBrushRadius = 0.5f;
			else if(TileBrushRadius > 20.5f)
				TileBrushRadius = 20.5f; }
		else {
			float Multiplier = 1.0f * Direction;
			if(IsShiftDown)
				Multiplier = 10.0f * Direction;

			// Zoom
			Camera->UpdateDistance(-Multiplier);
		}
	}
	else {
		if(Direction > 0)
			PaletteElement[CurrentPalette]->UpdateChildrenOffset(glm::ivec2(0, EDITOR_PALETTE_SIZE));
		else
			PaletteElement[CurrentPalette]->UpdateChildrenOffset(glm::ivec2(0, -EDITOR_PALETTE_SIZE));
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
	ZoneElement->Update(FrameTime, Input.GetMouse());
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

	// Update based on editor state
	switch(CurrentPalette) {
		case EDITMODE_TILES:
			if(IsDrawing) {
				if(Brush[EDITMODE_TILES]) {
					for(int j = 0; j < TileBrushRadius * 2; j++) {
						for(int i = 0; i < TileBrushRadius * 2; i++) {
							glm::ivec2 Offset(i - (int)TileBrushRadius, j - (int)TileBrushRadius);
							if(Offset.x * Offset.x + Offset.y * Offset.y > TileBrushRadius * TileBrushRadius)
								continue;

							glm::ivec2 TilePosition = Map->Grid->GetValidCoord(WorldCursor + glm::vec2(Offset));
							Map->Grid->Tiles[TilePosition.x][TilePosition.y].TextureIndex = Brush[EDITMODE_TILES]->TextureIndex;
						}
					}
				}
			}
		break;
		case EDITMODE_BLOCKS:
		case EDITMODE_ZONE:

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
		break;
		default:
		break;
	}

	// Handle object movement
	if(IsMoving) {
		for(auto &Object : SelectedObjects) {
			if(!Object->Physics)
				continue;

			Object->Physics->Position = glm::vec3(Map->GetValidPosition(glm::vec2(Object->Physics->NetworkPosition) + WorldCursor - ClickedPosition), Object->Physics->Position.z);
		}
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
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw floors
	Map->RenderFloors();

	// Draw objects
	Map->RenderObjects(BlendFactor, true);

	// Draw text over zones
	Graphics.SetDepthTest(false);
	for(auto &Object : Map->RenderList[Assets.Layers["zone"].Layer].Objects) {
		if(Object->Components.find("zone") != Object->Components.end()) {
			_Zone *Zone = (_Zone *)(Object->Components["zone"]);

			std::ostringstream Buffer;
			Buffer << Zone->OnEnter;
			Assets.Fonts["menu_buttons"]->DrawText(Buffer.str(), glm::vec2(Object->Physics->Position), COLOR_WHITE, CENTER_BASELINE, 1.0f / 64.0f);
		}
	}
	Graphics.SetDepthTest(true);

	// Draw tentative asset
	switch(CurrentPalette) {
		case EDITMODE_TILES:
			Graphics.SetColor(COLOR_WHITE);
			Graphics.SetProgram(Assets.Programs["pos"]);
			Graphics.SetVBO(VBO_CIRCLE);
			Graphics.SetDepthTest(false);
			Graphics.DrawCircle(glm::vec3(WorldCursor, 0.0f), TileBrushRadius);
			Graphics.SetDepthTest(true);
		break;
		case EDITMODE_BLOCKS:
			if(IsDrawing && Brush[CurrentPalette]) {
				Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
				Graphics.SetVBO(VBO_CUBE);
				glm::vec4 Color(COLOR_WHITE);
				Color.a *= 0.5f;
				Graphics.SetColor(Color);
				Graphics.SetDepthTest(false);
				Graphics.DrawCube(glm::vec3(DrawStart), glm::vec3(DrawEnd - DrawStart), Brush[CurrentPalette]->Style->Texture);
				Graphics.SetDepthTest(true);
			}
		break;
		case EDITMODE_OBJECTS:
		case EDITMODE_PROPS:
			if(Brush[CurrentPalette]) {
				_Object *Object = (_Object *)Brush[CurrentPalette]->UserData;
				if(!Object)
					break;

				// Set position
				Object->Physics->ForcePosition(WorldCursor);

				// Draw
				glm::vec4 Color(COLOR_WHITE);
				Color.a *= 0.5f;
				Object->Render->Color = Color;
				Object->Render->Draw3D(BlendFactor);
			}
		break;
		case EDITMODE_ZONE:
			if(IsDrawing && Brush[CurrentPalette]) {
				Graphics.SetProgram(Assets.Programs["pos"]);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(Brush[CurrentPalette]->Style->BackgroundColor);
				Graphics.SetDepthTest(false);
				Graphics.DrawRectangle(glm::vec2(DrawStart), glm::vec2(DrawEnd), true);
				Graphics.SetDepthTest(true);
			}
		break;
	}

	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetDepthTest(false);

	// Draw map boundaries
	Graphics.SetVBO(VBO_NONE);
	Graphics.SetColor(COLOR_RED);
	Graphics.DrawRectangle(glm::vec2(-0.01f, -0.01f), glm::vec2(Map->Grid->Size.x + 0.01f, Map->Grid->Size.y + 0.01f));

	// Draw grid
	glUniformMatrix4fv(Assets.Programs["pos"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	Map->RenderGrid(GridMode, GridVertices);

	// Outline the blocks
	if(HighlightBlocks)
		Map->HighlightBlocks();

	// Outline selected objects
	Graphics.SetColor(COLOR_WHITE);
	for(auto &Object : SelectedObjects) {
		if(!Object->Physics || !Object->Shape)
			continue;

		if(Object->Shape->IsAABB()) {
			glm::vec4 AABB = Object->Shape->GetAABB(Object->Physics->Position);
			Graphics.SetVBO(VBO_NONE);
			Graphics.DrawRectangle(glm::vec2(AABB[0], AABB[1]), glm::vec2(AABB[2], AABB[3]));
		}
		else {
			Graphics.SetVBO(VBO_CIRCLE);
			Graphics.DrawCircle(glm::vec3(Object->Physics->Position.x, Object->Physics->Position.y, Object->Render->Stats->Z), Object->Shape->HalfWidth[0]);
		}
	}

	// Dragging a box around object
	Graphics.SetVBO(VBO_NONE);
	if(DraggingBox) {
		Graphics.SetColor(COLOR_WHITE);
		Graphics.DrawRectangle(ClickedPosition, WorldCursor);
	}

	// Draw a block
	Graphics.SetColor(COLOR_GREEN);
	if(IsDrawing && CurrentPalette == EDITMODE_BLOCKS)
		Graphics.DrawRectangle(glm::vec2(DrawStart), glm::vec2(DrawEnd));

	// Setup 2D transformation
	Graphics.Setup2D();
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

	// Draw grid object count
	if(0) {
		glm::ivec2 Start(Camera->GetAABB()[0], Camera->GetAABB()[1]);
		glm::ivec2 End(Camera->GetAABB()[2], Camera->GetAABB()[3]);
		for(int X = Start.x; X < End.x; X++) {
			for(int Y = Start.y; Y < End.y; Y++) {
				if(X >= 0 && Y >= 0 && X < Map->Grid->Size.x && Y < Map->Grid->Size.y) {
					glm::ivec2 P;
					Camera->ConvertWorldToScreen(glm::vec2(X+0.5f, Y+0.5f), P);
					std::ostringstream Buffer;
					Buffer << Map->Grid->Tiles[X][Y].Objects.size();
					Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), P);
					Buffer.str("");
				}
			}
		}
	}

	// Draw viewport outline
	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.SetColor(COLOR_DARK);
	Graphics.DrawRectangle(glm::vec2(0, 0), glm::vec2(Graphics.ViewportSize.x, Graphics.ViewportSize.y));

	// Draw text
	if(EditorInputType != -1) {
		InputBox->Render();
	}

	int X = 15;
	int Y = (float)Graphics.WindowSize.y - 25;

	Graphics.SetProgram(Assets.Programs["text"]);
	Graphics.SetVBO(VBO_NONE);

	std::ostringstream Buffer;
	Buffer << Map->Filename;
	MainFont->DrawText(Buffer.str(), glm::vec2(X, 25));
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
	else if(CurrentPalette == EDITMODE_ZONE) {
		ZoneElement->Render();
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
			Palette.push_back(_Palette("block", Assets.Textures[Identifier]->Identifier, nullptr, Assets.Textures[Identifier], nullptr, 0, COLOR_WHITE));
		}

		LoadPaletteButtons(Palette, EDITMODE_BLOCKS);
	}

	{
		// Load objects
		std::vector<_Palette> Palette;
		std::vector<_Palette> PaletteProps;
		for(auto &Iterator : Stats->Objects) {
			const auto &RenderIterator = Iterator.second.Components.find("render");
			if(RenderIterator != Iterator.second.Components.end()) {
				const _ObjectStat &ObjectStat = Iterator.second;
				const _RenderStat *RenderStat = (const _RenderStat *)RenderIterator->second.get();
				const auto &PhysicsIterator = ObjectStat.Components.find("physics");
				if(RenderStat->Layer == Assets.Layers["block"].Layer ||
				   RenderStat->Layer == Assets.Layers["zone"].Layer ||
				   PhysicsIterator == ObjectStat.Components.end())
					continue;

				// Create object
				_Object *Object = new _Object();

				// Add components
				_Render *Render = new _Render(Object, RenderStat);
				_Physics *Physics = new _Physics(Object, (const _PhysicsStat *)PhysicsIterator->second.get());

				Object->Render = Render;
				Object->Physics = Physics;
				Object->Components["render"] = Render;
				Object->Components["physics"] = Physics;
				Object->Render->Program = Assets.Programs[RenderStat->ProgramIdentifier];
				Object->Render->Texture = Assets.Textures[RenderStat->TextureIdentifier];
				Object->Render->Mesh = Assets.Meshes[RenderStat->MeshIdentifier];

				if(!Object->Render->Mesh)
					Palette.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Name, Object, Object->Render->Texture, nullptr, 0, COLOR_WHITE));
				else
					PaletteProps.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Identifier, Object, Object->Render->Texture, nullptr, 0, COLOR_WHITE));
			}
		}

		LoadPaletteButtons(Palette, EDITMODE_OBJECTS);
		LoadPaletteButtons(PaletteProps, EDITMODE_PROPS);
	}

	// Load zones
	{
		// Load objects
		std::vector<_Palette> Palette;
		for(auto &Iterator : Stats->Objects) {
			const auto &RenderIterator = Iterator.second.Components.find("render");
			if(RenderIterator != Iterator.second.Components.end()) {
				const _ObjectStat &ObjectStat = Iterator.second;
				const _RenderStat *RenderStat = (const _RenderStat *)RenderIterator->second.get();
				const auto &PhysicsIterator = ObjectStat.Components.find("physics");
				if(RenderStat->Layer != Assets.Layers["zone"].Layer || !RenderStat || PhysicsIterator == ObjectStat.Components.end())
					continue;

				// Create object
				_Object *Object = new _Object();

				// Add components
				_Render *Render = new _Render(Object, RenderStat);
				_Physics *Physics = new _Physics(Object, (const _PhysicsStat *)PhysicsIterator->second.get());
				Object->Render = Render;
				Object->Physics = Physics;
				Object->Components["render"] = Render;
				Object->Components["physics"] = Physics;
				Object->Render->Program = Assets.Programs[RenderStat->ProgramIdentifier];
				Object->Render->Color = Assets.Colors[RenderStat->ColorIdentifier];

				Palette.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Name, Object, Object->Render->Texture, nullptr, 0, Object->Render->Color));
			}
		}

		LoadPaletteButtons(Palette, EDITMODE_ZONE);
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
		Style->BackgroundColor = Palette[i].Color;
		Style->BorderColor = Palette[i].Color;
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
		Button->Size = glm::ivec2(EDITOR_PALETTE_SIZE, EDITOR_PALETTE_SIZE);
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->HoverStyle = Assets.Styles["style_editor_selected0"];
		Button->UserData = Palette[i].UserData;
		PaletteElement[Type]->Children.push_back(Button);

		// Assign texture index for atlases
		Button->TextureIndex = Palette[i].TextureIndex;

		// Update position
		Offset.x += EDITOR_PALETTE_SIZE;
		if(Offset.x > Width - EDITOR_PALETTE_SIZE) {
			Offset.y += EDITOR_PALETTE_SIZE;
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
			int X = (float)Graphics.ViewportSize.x + 100;
			int Y = (float)Graphics.ViewportSize.y + 5;

			std::ostringstream Buffer;
			Buffer << DrawStart.z;
			MainFont->DrawText("Min Z:", glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");

			Buffer << DrawEnd.z;
			MainFont->DrawText("Max Z:", glm::vec2(X + 85, Y), COLOR_WHITE, RIGHT_BASELINE);
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 90, Y));
			Buffer.str("");
		} break;
		default:

			// See if there's a selected object
			if(SelectedObjects.size() > 0) {
				auto &Object = *SelectedObjects.begin();
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
	}
}

// Executes the walkable command
void _EditorState::ExecuteWalkable(_EditorState *State, _Element *Element) {
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
		State->DrawStart.z += Change;
	}
	else {
		State->DrawEnd.z += Change;
	}
}

// Executes the deselect command
void _EditorState::ExecuteDeselect(_EditorState *State, _Element *Element) {
	State->SelectedObjects.clear();
}

// Executes the delete command
void _EditorState::ExecuteDelete(_EditorState *State, _Element *Element) {
	if(State->ObjectsSelected()) {
		for(auto &Object : State->SelectedObjects)
			Object->Deleted = true;

		State->SelectedObjects.clear();
		State->ClipboardObjects.clear();
	}
}

// Executes the copy command
void _EditorState::ExecuteCopy(_EditorState *State, _Element *Element) {
	if(State->ObjectsSelected()) {
		State->CopiedPosition = State->WorldCursor;
		State->ClipboardObjects = State->SelectedObjects;
	}
}

// Executes the paste command
void _EditorState::ExecutePaste(_EditorState *State, _Element *Element) {
	glm::vec2 StartPosition;

	if(Element)
		StartPosition = glm::vec2(State->Camera->GetPosition());
	else
		StartPosition = State->WorldCursor;

	for(auto &Iterator : State->ClipboardObjects) {
		_Object *Object = State->Stats->CreateObject(Iterator->Identifier, false);
		if(Object->Physics) {
			glm::vec2 NewPosition = State->Map->GetValidPosition(StartPosition - State->CopiedPosition + glm::vec2(Iterator->Physics->Position));
			Object->Physics->LastPosition = Object->Physics->Position = glm::vec3(NewPosition, Iterator->Physics->Position.z);
		}

		if(Object->Render)
			Object->Render->Texture = Iterator->Render->Texture;

		if(Object->Shape)
			Object->Shape->HalfWidth = Iterator->Shape->HalfWidth;

		Object->Map = State->Map;
		State->Map->AddObject(Object);
		State->Map->Grid->AddObject(Object);
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

	// Make sure objects are selected for script callbacks
	if(Type == 2 && State->SelectedObjects.size() == 0) {
		return;
	}

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
	if(Button->Style == 0) {
		Brush[CurrentPalette] = nullptr;
		return;
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
	for(auto &Object : SelectedObjects) {
		Object->Physics->NetworkPosition = Object->Physics->Position;
	}

	IsMoving = false;
}

// Cancel a move operation
void _EditorState::CancelMove() {
	for(auto &Object : SelectedObjects) {
		Object->Physics->Position = Object->Physics->NetworkPosition;
	}

	IsMoving = false;
}
