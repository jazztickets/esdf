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
#include <states/editor.h>
#include <states/client.h>
#include <ae/manager.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/render.h>
#include <objects/shape.h>
#include <objects/zone.h>
#include <framework.h>
#include <ae/graphics.h>
#include <stats.h>
#include <ae/camera.h>
#include <ae/input.h>
#include <ae/font.h>
#include <ae/assets.h>
#include <map.h>
#include <grid.h>
#include <menu.h>
#include <ae/texture.h>
#include <ae/atlas.h>
#include <config.h>
#include <constants.h>
#include <ae/files.h>
#include <ae/mesh.h>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <SDL_keycode.h>
#include <SDL_mouse.h>

#include <ae/program.h>
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

	Stats = new _Stats();
	ObjectManager = new ae::_Manager<_Object>();

	// Load command buttons
	MainFont = ae::Assets.Fonts["hud_medium"];
	CommandElement = ae::Assets.Elements["element_editor_command"];
	BlockElement = ae::Assets.Elements["element_editor_blocks"];
	ZoneElement = ae::Assets.Elements["element_editor_zone"];
	InputBox = ae::Assets.Elements["element_editor_input"];
	CommandElement->SetActive(true);
	BlockElement->SetActive(true);
	ZoneElement->SetActive(true);
	InputBox->SetActive(false);

	// Create button groups
	PaletteElement[0] = ae::Assets.Elements["element_editor_palette_tile"];
	PaletteElement[1] = ae::Assets.Elements["element_editor_palette_block"];
	PaletteElement[2] = ae::Assets.Elements["element_editor_palette_object"];
	PaletteElement[3] = ae::Assets.Elements["element_editor_palette_prop"];
	PaletteElement[4] = ae::Assets.Elements["element_editor_palette_zone"];

	// Assign palette buttons
	ModeButtons[0] = ae::Assets.Elements["button_editor_mode_tile"];
	ModeButtons[1] = ae::Assets.Elements["button_editor_mode_block"];
	ModeButtons[2] = ae::Assets.Elements["button_editor_mode_object"];
	ModeButtons[3] = ae::Assets.Elements["button_editor_mode_prop"];
	ModeButtons[4] = ae::Assets.Elements["button_editor_mode_zone"];
	for(int i = 0; i < EDITMODE_COUNT; i++) {
		ModeButtons[i]->SetActive(true);
	}

	// Reset state
	ResetState();

	// Create camera
	Camera = new ae::_Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_EDITOR_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);

	// Load level
	if(ClientState.GetFromEditor())
		MapFilename = EDITOR_TESTLEVEL;

	LoadMap(MapFilename, ClientState.GetFromEditor());

	// Set up graphics
	ae::Graphics.ChangeViewport(ae::Graphics.CurrentSize - EDITOR_VIEWPORT_OFFSET);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	ae::Graphics.ShowCursor(ae::CURSOR_MAIN);

	// Enable last palette
	if(SavedPalette != -1) {
		PaletteElement[0]->SetActive(false);
		CurrentPalette = SavedPalette;
		ModeButtons[0]->Checked = false;
		ModeButtons[SavedPalette]->Checked = true;
		PaletteElement[SavedPalette]->SetActive(true);
	}
}

void _EditorState::Close() {
	SavedCameraPosition = Camera->GetPosition();
	SavedPalette = CurrentPalette;

	for(int i = 0; i < EDITMODE_COUNT; i++)
		ClearPalette(i);

	delete Stats;
	delete ObjectManager;
	delete Camera;
	delete Map;

	Camera = nullptr;
	Map = nullptr;
}

// Load a level
bool _EditorState::LoadMap(const std::string &File, bool UseSavedCameraPosition) {
	if(Map)
		delete Map;

	ObjectManager->Clear();
	Map = new _Map();
	Map->Load(File, Stats, ObjectManager);
	Map->SetCamera(Camera);

	// Set up editor state
	ResetState();

	// Set camera
	if(UseSavedCameraPosition)
		Camera->ForcePosition(SavedCameraPosition);
	else
		Camera->ForcePosition(glm::vec3(Map->GetStartingPositionByCheckpoint(0), CAMERA_DISTANCE));

	// Load tileset
	std::vector<_Palette> Palette;
	uint32_t TextureCount = (uint32_t)(Map->TileAtlas->Texture->Size.x * Map->TileAtlas->Texture->Size.y / (Map->TileAtlas->Size.x * Map->TileAtlas->Size.y));
	for(uint32_t i = 0; i < TextureCount; i++) {
		Palette.push_back(_Palette(std::to_string(i), std::to_string(i), nullptr, nullptr, Map->TileAtlas, nullptr, i, glm::vec4(1.0f)));
	}

	LoadPaletteButtons(Palette, EDITMODE_TILES);
	PaletteElement[CurrentPalette]->SetActive(true);

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
	ae::Assets.Elements["button_editor_show"]->Checked = false;
	BlockElement->SetActive(false);
	ZoneElement->SetActive(false);

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
		ModeButtons[i]->Checked = false;
		Brush[i] = nullptr;
	}

	// Load palettes
	LoadPalettes();
	ModeButtons[CurrentPalette]->Checked = true;
	InputBox->SetActive(false);
}

// Action handler
bool _EditorState::HandleAction(int InputType, size_t Action, int Value) {

	return false;
}

// Key handler
bool _EditorState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	if(IsDrawing || !KeyEvent.Pressed)
		return true;

	if(IgnoreTextEvent) {
		IgnoreTextEvent = false;
		return true;
	}

	// See if the user is entering in text
	if(EditorInputType != -1) {
		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_RETURN: {
				const std::string InputText = InputBox->Children.front()->Text;
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
							if(Object->HasComponent("zone")) {
								_Zone *Zone = (_Zone *)(Object->Components["zone"]);
								Zone->OnEnter = InputText;
							}
						}

						//SavedText[EditorInputType] = InputText;
					break;
				}
				EditorInputType = -1;
				InputBox->SetActive(false);
			} break;
			case SDL_SCANCODE_ESCAPE:
				EditorInputType = -1;
				InputBox->SetActive(false);
			break;
			default:
				InputBox->HandleKey(KeyEvent);
			break;
		}
	}
	else {

		// Command keys
		switch(KeyEvent.Scancode) {

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
				ExecuteSwitchMode(this, ae::Assets.Elements["button_editor_mode_tile"]);
			break;
			case SDL_SCANCODE_2:
				ExecuteSwitchMode(this, ae::Assets.Elements["button_editor_mode_block"]);
			break;
			case SDL_SCANCODE_3:
				ExecuteSwitchMode(this, ae::Assets.Elements["button_editor_mode_object"]);
			break;
			case SDL_SCANCODE_4:
				ExecuteSwitchMode(this, ae::Assets.Elements["button_editor_mode_prop"]);
			break;
			case SDL_SCANCODE_5:
				ExecuteSwitchMode(this, ae::Assets.Elements["button_editor_mode_zone"]);
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
				ExecuteHighlightBlocks(this, ae::Assets.Elements["button_editor_show"]);
			break;
			case SDL_SCANCODE_A:
				if(CurrentPalette == EDITMODE_BLOCKS)
					ExecuteWalkable(this, nullptr);
			break;
			case SDL_SCANCODE_E:
				if(CurrentPalette == EDITMODE_ZONE) {
					ExecuteIOCommand(this, ae::Assets.Elements["button_editor_onenter"]);
					IgnoreTextEvent = true;
				}
			break;
			case SDL_SCANCODE_KP_MINUS:
				ExecuteChangeZ(this, ae::Assets.Elements["button_editor_lower"]);
			break;
			case SDL_SCANCODE_KP_PLUS:
				ExecuteChangeZ(this, ae::Assets.Elements["button_editor_raise"]);
			break;
			case SDL_SCANCODE_N:
				if(IsCtrlDown)
					ExecuteNew(this, nullptr);
			break;
			case SDL_SCANCODE_L:
				ExecuteIOCommand(this, ae::Assets.Elements["button_editor_load"]);
				IgnoreTextEvent = true;
			break;
			case SDL_SCANCODE_S:
				ExecuteIOCommand(this, ae::Assets.Elements["button_editor_save"]);
				IgnoreTextEvent = true;
			break;
			case SDL_SCANCODE_T:
				ExecuteTest(this, nullptr);
			break;
		}
	}

	return true;
}

// Mouse handler
void _EditorState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	ae::FocusedElement = nullptr;
	ae::Graphics.Element->HandleMouseButton(MouseEvent.Pressed);

	// Handle command group clicks
	ae::_Element *Clicked = CommandElement->GetClickedElement();
	if(Clicked && (intptr_t)Clicked->UserData != -1) {
		if(IconCallbacks.find(Clicked->Name) != IconCallbacks.end())
			IconCallbacks[Clicked->Name](this, Clicked);
	}

	if(CurrentPalette == EDITMODE_BLOCKS) {
		ae::_Element *Clicked = BlockElement->GetClickedElement();
		if(Clicked && (intptr_t)Clicked->UserData != -1) {
			if(IconCallbacks.find(Clicked->Name) != IconCallbacks.end())
				IconCallbacks[Clicked->Name](this, Clicked);
		}
	}
	else if(CurrentPalette == EDITMODE_ZONE) {
		ae::_Element *Clicked = ZoneElement->GetClickedElement();
		if(Clicked && (intptr_t)Clicked->UserData != -1) {
			if(IconCallbacks.find(Clicked->Name) != IconCallbacks.end())
				IconCallbacks[Clicked->Name](this, Clicked);
		}
	}

	// Handle viewport clicks
	if(ae::Input.GetMouse().x < ae::Graphics.ViewportSize.x && ae::Input.GetMouse().y < ae::Graphics.ViewportSize.y) {
		if(MouseEvent.Pressed) {
			switch(MouseEvent.Button) {
				case SDL_BUTTON_LEFT:
					if(!IsMoving && !Clicked) {
						ae::_Element *Button = Brush[CurrentPalette];

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

									_Object *Object = ObjectManager->Create();
									Stats->CreateObject(Object, Button->Name, false);
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
		if(!MouseEvent.Pressed) {

			// Get button click for palette
			ae::_Element *Button = PaletteElement[CurrentPalette]->GetClickedElement();
			if(Button && Button != PaletteElement[CurrentPalette]) {
				ExecuteSelectPalette(Button, MouseEvent.Button == SDL_BUTTON_RIGHT);
			}
		}
	}

	// Mouse release
	if(!MouseEvent.Pressed) {
		switch(MouseEvent.Button) {
			case SDL_BUTTON_LEFT:
				if(IsDrawing) {
					ae::_Element *Button = Brush[CurrentPalette];

					switch(CurrentPalette) {
						case EDITMODE_BLOCKS:
							if(Button) {
								_Object *Object = ObjectManager->Create();
								Stats->CreateObject(Object, Button->Name, false);
								Object->Map = Map;
								Object->Render->Texture = Button->Texture;
								Object->Physics->LastPosition = Object->Physics->Position = (DrawStart + DrawEnd) / 2.0f;
								Object->Shape->HalfWidth = (DrawEnd - DrawStart) / 2.0f;
								Map->AddObject(Object);
								Map->Grid->AddObject(Object);
							}
						break;
						case EDITMODE_ZONE:
							if(Button) {
								_Object *Object = ObjectManager->Create();
								Stats->CreateObject(Object, Button->Name, false);
								Object->Map = Map;
								Object->Physics->LastPosition = Object->Physics->Position = (DrawStart + DrawEnd) / 2.0f;
								Object->Shape->HalfWidth = (DrawEnd - DrawStart) / 2.0f;
								Object->Shape->HalfWidth.z = 0.0f;
								Object->Physics->LastPosition.z = Object->Physics->Position.z = 0.0f;
								Map->AddObject(Object);
								Map->Grid->AddObject(Object);
							}
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
					Map->GetSelectedObjects(AABB, Selection);

					// Filter selection by palette type
					SelectedObjects.clear();
					for(auto &Object : Selection) {
						switch(CurrentPalette) {
							case EDITMODE_BLOCKS:
								if(Object->Render->Stats->Layer == ae::Assets.Layers["block"].Layer)
									SelectedObjects.push_back(Object);
							break;
							case EDITMODE_OBJECTS:
								if(Object->Render->Stats->Layer != ae::Assets.Layers["block"].Layer && Object->Render->Stats->Layer != ae::Assets.Layers["zone"].Layer && !Object->Render->Mesh)
									SelectedObjects.push_back(Object);
							break;
							case EDITMODE_PROPS:
								if(Object->Render->Stats->Layer != ae::Assets.Layers["block"].Layer && Object->Render->Stats->Layer != ae::Assets.Layers["zone"].Layer && Object->Render->Mesh)
									SelectedObjects.push_back(Object);
							break;
							case EDITMODE_ZONE:
								if(Object->Render->Stats->Layer == ae::Assets.Layers["zone"].Layer)
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
void _EditorState::HandleMouseWheel(int Direction) {

	if(ae::Input.GetMouse().x < ae::Graphics.ViewportSize.x && ae::Input.GetMouse().y < ae::Graphics.ViewportSize.y) {
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

void _EditorState::HandleWindow(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(ae::Graphics.AspectRatio);
}

// Update
void _EditorState::Update(double FrameTime) {
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());
	//if(ae::Graphics.Element->HitElement)
	//	std::cout << ae::Graphics.Element->HitElement->Name << std::endl;

	// Get modifier key status
	IsShiftDown = ae::Input.ModKeyDown(KMOD_SHIFT) ? true : false;
	IsCtrlDown = ae::Input.ModKeyDown(KMOD_CTRL) ? true : false;

	if(IsShiftDown)
		AlignDivisor = 2;
	else
		AlignDivisor = EDITOR_ALIGN_DIVISOR;

	// Get world cursor
	Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);

	// Get tile indices for later usage
	WorldCursor = AlignToGrid(Map->GetValidPosition(WorldCursor));

	// Set camera position
	Camera->Update(FrameTime);

	// Update objects
	ObjectManager->Update(FrameTime);

	// Update map
	Map->Update(FrameTime);

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

	ae::Assets.Programs["pos_uv"]->LightCount = 0;
	ae::Assets.Programs["pos_uv"]->AmbientLight = AmbientLight;
	ae::Assets.Programs["pos_uv_norm"]->LightCount = 0;
	ae::Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLight;

	// Setup 3D transformation
	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw floors
	Map->RenderFloors();

	// Draw objects
	Map->RenderObjects(BlendFactor, true);

	// Draw text over zones
	ae::Graphics.SetDepthTest(false);
	for(auto &Object : Map->RenderList[(size_t)ae::Assets.Layers["zone"].Layer].Objects) {
		if(Object->HasComponent("zone")) {
			_Zone *Zone = (_Zone *)(Object->Components["zone"]);

			std::ostringstream Buffer;
			Buffer << Zone->OnEnter;
			ae::Assets.Fonts["menu_buttons"]->DrawText(Buffer.str(), glm::vec2(Object->Physics->Position), ae::CENTER_BASELINE, glm::vec4(1.0f), 1.0f / 64.0f);
		}
	}
	ae::Graphics.SetDepthTest(true);

	// Draw tentative asset
	switch(CurrentPalette) {
		case EDITMODE_TILES:
			ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
			ae::Graphics.SetColor(glm::vec4(1.0f));
			ae::Graphics.SetDepthTest(false);
			ae::Graphics.DrawCircle(glm::vec3(WorldCursor, 0.0f), TileBrushRadius);
			ae::Graphics.SetDepthTest(true);
		break;
		case EDITMODE_BLOCKS:
			if(IsDrawing && Brush[CurrentPalette]) {
				ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_norm"]);
				glm::vec4 Color(glm::vec4(1.0f));
				Color.a *= 0.5f;
				ae::Graphics.SetColor(Color);
				ae::Graphics.SetDepthTest(false);
				ae::Graphics.DrawCube(glm::vec3(DrawStart), glm::vec3(DrawEnd - DrawStart), Brush[CurrentPalette]->Texture);
				ae::Graphics.SetDepthTest(true);
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
				glm::vec4 Color(glm::vec4(1.0f));
				Color.a *= 0.5f;
				Object->Render->Color = Color;
				Object->Render->Draw3D(BlendFactor);
			}
		break;
		case EDITMODE_ZONE:
			if(IsDrawing && Brush[CurrentPalette]) {
				ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
				ae::Graphics.SetColor(Brush[CurrentPalette]->Color);
				ae::Graphics.SetDepthTest(false);
				ae::Graphics.DrawRectangle3D(glm::vec2(DrawStart), glm::vec2(DrawEnd), true);
				ae::Graphics.SetDepthTest(true);
			}
		break;
	}

	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	ae::Graphics.SetDepthTest(false);

	// Draw map boundaries
	ae::Graphics.SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	ae::Graphics.DrawRectangle3D(glm::vec2(0), glm::vec2(Map->Grid->Size), false);

	// Draw grid
	glUniformMatrix4fv(ae::Assets.Programs["pos"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	Map->RenderGrid(GridMode);
	ae::Graphics.DirtyState();
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);

	// Outline the blocks
	if(HighlightBlocks)
		Map->HighlightBlocks();

	// Outline selected objects
	ae::Graphics.SetColor(glm::vec4(1.0f));
	for(auto &Object : SelectedObjects) {
		if(!Object->Physics || !Object->Shape)
			continue;

		if(Object->Shape->IsAABB()) {
			glm::vec4 AABB = Object->Shape->GetAABB(Object->Physics->Position);
			ae::Graphics.DrawRectangle3D(glm::vec2(AABB[0], AABB[1]), glm::vec2(AABB[2], AABB[3]), false);
		}
		else {
			ae::Graphics.DrawCircle(glm::vec3(Object->Physics->Position.x, Object->Physics->Position.y, Object->Render->Stats->Z), Object->Shape->HalfWidth[0]);
		}
	}

	// Dragging a box around object
	if(DraggingBox) {
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawRectangle3D(ClickedPosition, WorldCursor, false);
	}

	// Draw a block
	ae::Graphics.SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	if(IsDrawing && CurrentPalette == EDITMODE_BLOCKS)
		ae::Graphics.DrawRectangle3D(glm::vec2(DrawStart), glm::vec2(DrawEnd), false);

	// Setup 2D transformation
	ae::Graphics.Setup2D();
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));

	// Draw grid object count
	if(0) {
		glm::ivec2 Start(Camera->GetAABB()[0], Camera->GetAABB()[1]);
		glm::ivec2 End(Camera->GetAABB()[2], Camera->GetAABB()[3]);
		for(int X = Start.x; X < End.x; X++) {
			for(int Y = Start.y; Y < End.y; Y++) {
				if(X >= 0 && Y >= 0 && X < Map->Grid->Size.x && Y < Map->Grid->Size.y) {
					glm::vec2 P;
					Camera->ConvertWorldToScreen(glm::vec2(X+0.5f, Y+0.5f), P);
					std::ostringstream Buffer;
					Buffer << Map->Grid->Tiles[X][Y].Objects.size();
					ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), P);
					Buffer.str("");
				}
			}
		}
	}

	// Draw viewport outline
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
	ae::Graphics.DrawRectangle(glm::vec2(0, 0), glm::vec2(ae::Graphics.ViewportSize.x, ae::Graphics.ViewportSize.y));

	float X = 15;
	float Y = (float)ae::Graphics.CurrentSize.y - 25;

	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);

	std::ostringstream Buffer;
	Buffer << Map->Filename;
	MainFont->DrawText(Buffer.str(), glm::vec2(X, 25));
	Buffer.str("");

	Buffer << std::fixed << std::setprecision(1) << WorldCursor.x << ", " << WorldCursor.y;
	MainFont->DrawText(Buffer.str(), glm::vec2(X, (float)ae::Graphics.ViewportSize.y - 25));
	Buffer.str("");

	X = (float)ae::Graphics.ViewportSize.x - 45;

	Y = (float)25;
	Buffer << ae::Graphics.FramesPerSecond;
	MainFont->DrawText("FPS:", glm::vec2(X, Y), ae::RIGHT_BASELINE, glm::vec4(1.0f));
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	Y = (float)ae::Graphics.ViewportSize.y - 40;
	Buffer << CheckpointIndex;
	MainFont->DrawText("Checkpoint:", glm::vec2(X, Y), ae::RIGHT_BASELINE, glm::vec4(1.0f));
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	Y += 15;
	Buffer << GridMode;
	MainFont->DrawText("Grid:", glm::vec2(X, Y), ae::RIGHT_BASELINE, glm::vec4(1.0f));
	MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
	Buffer.str("");

	// Draw current brush
	DrawBrush();

	ae::Graphics.Element->Render();
}

// Load palette buttons
void _EditorState::LoadPalettes() {

	// Load block textures
	{
		std::vector<_Palette> Palette;
		ae::_Files Files("textures/blocks/");
		for(const auto &File : Files.Nodes) {
			std::string Identifier = "textures/blocks/" + File;
			Palette.push_back(_Palette("block", ae::Assets.Textures[Identifier]->Name, nullptr, ae::Assets.Textures[Identifier], nullptr, nullptr, 0, glm::vec4(1.0f)));
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
				if(RenderStat->Layer == ae::Assets.Layers["block"].Layer ||
				   RenderStat->Layer == ae::Assets.Layers["zone"].Layer ||
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
				Object->Render->Program = ae::Assets.Programs[RenderStat->ProgramIdentifier];
				Object->Render->Texture = ae::Assets.Textures[RenderStat->TextureIdentifier];
				Object->Render->Mesh = ae::Assets.Meshes[RenderStat->MeshIdentifier];

				if(!Object->Render->Mesh)
					Palette.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Name, Object, Object->Render->Texture, nullptr, nullptr, 0, glm::vec4(1.0f)));
				else
					PaletteProps.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Identifier, Object, Object->Render->Texture, nullptr, nullptr, 0, glm::vec4(1.0f)));
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
				if(RenderStat->Layer != ae::Assets.Layers["zone"].Layer || !RenderStat || PhysicsIterator == ObjectStat.Components.end())
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
				Object->Render->Program = ae::Assets.Programs[RenderStat->ProgramIdentifier];
				Object->Render->Color = ae::Assets.Colors[RenderStat->ColorIdentifier];

				Palette.push_back(_Palette(ObjectStat.Identifier, ObjectStat.Name, Object, Object->Render->Texture, nullptr, ae::Assets.Styles["style_editor_zone"], 0, Object->Render->Color));
			}
		}

		LoadPaletteButtons(Palette, EDITMODE_ZONE);
	}
}

// Free memory used by palette
void _EditorState::ClearPalette(int Type) {
	std::list<ae::_Element *> &Children = PaletteElement[Type]->Children;
	for(const auto &Child : Children) {
		delete (_Object *)(Child->UserData);
		delete Child;
	}

	Children.clear();
}

// Loads the palette
void _EditorState::LoadPaletteButtons(const std::vector<_Palette> &Palette, int Type) {
	ClearPalette(Type);

	// Loop through textures
	glm::vec2 Offset(0, 0);
	float Width = PaletteElement[Type]->Size.x;
	for(size_t i = 0; i < Palette.size(); i++) {

		// Add palette button
		ae::_Element *Button = new ae::_Element();
		Button->Name = Palette[i].Identifier;
		Button->Parent = PaletteElement[Type];
		Button->Offset = Offset;
		Button->Size = glm::vec2(EDITOR_PALETTE_SIZE, EDITOR_PALETTE_SIZE);
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Palette[i].Texture;
		Button->Color = Palette[i].Color;
		Button->Atlas = Palette[i].Atlas;
		Button->Style = Palette[i].Style;
		Button->HoverStyle = ae::Assets.Styles["style_editor_button_selected"];
		Button->UserData = Palette[i].UserData;
		Button->TextureIndex = Palette[i].TextureIndex;
		PaletteElement[Type]->Children.push_back(Button);

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
	glm::vec4 IconColor = glm::vec4(1.0f);
	const ae::_Texture *IconTexture = nullptr;
	const ae::_Atlas *IconAtlas = nullptr;
	uint32_t IconTextureIndex = 0;
	if(Brush[CurrentPalette]) {
		IconIdentifier = Brush[CurrentPalette]->Name;
		IconText = Brush[CurrentPalette]->Name;
		IconTexture = Brush[CurrentPalette]->Texture;
		IconAtlas = Brush[CurrentPalette]->Atlas;
		IconTextureIndex = Brush[CurrentPalette]->TextureIndex;
		IconColor = Brush[CurrentPalette]->Color;
	}

	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);

	// Edit mode specific text
	switch(CurrentPalette) {
		case EDITMODE_BLOCKS: {
			float X = (float)ae::Graphics.ViewportSize.x + 100;
			float Y = (float)ae::Graphics.ViewportSize.y + 5;

			std::ostringstream Buffer;
			Buffer << DrawStart.z;
			MainFont->DrawText("Min Z:", glm::vec2(X, Y), ae::RIGHT_BASELINE, glm::vec4(1.0f));
			MainFont->DrawText(Buffer.str(), glm::vec2(X + 5, Y));
			Buffer.str("");

			Buffer << DrawEnd.z;
			MainFont->DrawText("Max Z:", glm::vec2(X + 85, Y), ae::RIGHT_BASELINE, glm::vec4(1.0f));
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
		MainFont->DrawText(IconText, glm::vec2(ae::Graphics.ViewportSize) + glm::vec2(112, 130), ae::CENTER_MIDDLE, glm::vec4(1.0f));

	if(IconIdentifier != "")
		MainFont->DrawText(IconIdentifier, glm::vec2(ae::Graphics.ViewportSize) + glm::vec2(112, 145), ae::CENTER_MIDDLE, glm::vec4(1.0f));

	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);

	ae::_Bounds Bounds;
	Bounds.Start = ae::Graphics.CurrentSize - glm::ivec2(112, 84) - glm::ivec2(32);
	Bounds.End = Bounds.Start + glm::vec2(64);

	ae::Graphics.SetColor(IconColor);
	if(IconTexture)
		ae::Graphics.DrawImage(Bounds, IconTexture);
	else if(IconAtlas)
		ae::Graphics.DrawAtlas(Bounds, IconAtlas->Texture, IconAtlas->GetTextureCoords(IconTextureIndex));
}

// Executes the toggle editor mode
void _EditorState::ExecuteSwitchMode(_EditorState *State, ae::_Element *Element) {
	int Palette = Element->Index;

	// Toggle icons
	if(State->CurrentPalette != Palette) {
		State->PaletteElement[State->CurrentPalette]->SetActive(false);
		State->ModeButtons[State->CurrentPalette]->Checked = false;
		State->ModeButtons[Palette]->Checked = true;

		// Set state
		State->CurrentPalette = Palette;
		State->PaletteElement[Palette]->SetActive(true);
		State->BlockElement->SetActive(false);
		State->ZoneElement->SetActive(false);
		switch(State->CurrentPalette) {
			case EDITMODE_BLOCKS:
				State->BlockElement->SetActive(true);
			break;
			case EDITMODE_ZONE:
				State->ZoneElement->SetActive(true);
			break;
		}
	}
}

// Executes the walkable command
void _EditorState::ExecuteWalkable(_EditorState *State, ae::_Element *Element) {
}

// Executes the change z command
void _EditorState::ExecuteChangeZ(_EditorState *State, ae::_Element *Element) {
	float Change = -0.5f;
	if(Element->Index)
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
void _EditorState::ExecuteDeselect(_EditorState *State, ae::_Element *Element) {
	State->SelectedObjects.clear();
}

// Executes the delete command
void _EditorState::ExecuteDelete(_EditorState *State, ae::_Element *Element) {
	if(State->ObjectsSelected()) {
		for(auto &Object : State->SelectedObjects)
			Object->Deleted = true;

		State->SelectedObjects.clear();
		State->ClipboardObjects.clear();
	}
}

// Executes the copy command
void _EditorState::ExecuteCopy(_EditorState *State, ae::_Element *Element) {
	if(State->ObjectsSelected()) {
		State->CopiedPosition = State->WorldCursor;
		State->ClipboardObjects = State->SelectedObjects;
	}
}

// Executes the paste command
void _EditorState::ExecutePaste(_EditorState *State, ae::_Element *Element) {
	glm::vec2 StartPosition;

	if(Element)
		StartPosition = glm::vec2(State->Camera->GetPosition());
	else
		StartPosition = State->WorldCursor;

	for(auto &Iterator : State->ClipboardObjects) {
		_Object *Object = State->ObjectManager->Create();
		State->Stats->CreateObject(Object, Iterator->Identifier, false);
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
void _EditorState::ExecuteHighlightBlocks(_EditorState *State, ae::_Element *Element) {
	State->HighlightBlocks = !State->HighlightBlocks;

	// Toggle button state
	Element->Checked = State->HighlightBlocks;
}

// Executes the clear map command
void _EditorState::ExecuteNew(_EditorState *State, ae::_Element *Element) {
	State->LoadMap("", false);
	State->SavedText[EDITINPUT_SAVE] = "";
}

// Executes the an I/O command
void _EditorState::ExecuteIOCommand(_EditorState *State, ae::_Element *Element) {

	// Get io type
	int Type = Element->Index;

	// Make sure objects are selected for script callbacks
	if(Type == 2 && State->SelectedObjects.size() == 0) {
		return;
	}

	State->EditorInputType = Type;
	State->InputBox->SetActive(true);
	ae::_Element *TextBox = State->InputBox->Children.front();
	ae::_Element *Label = TextBox->Children.front();
	Label->Text = InputBoxStrings[Type];
	TextBox->Text = State->SavedText[Type];
	ae::FocusedElement = TextBox;
}

// Executes the test command
void _EditorState::ExecuteTest(_EditorState *State, ae::_Element *Element) {
	State->Map->Save(EDITOR_TESTLEVEL);

	ClientState.SetTestMode(true);
	ClientState.SetFromEditor(true);
	ClientState.SetLevel(EDITOR_TESTLEVEL);
	ClientState.SetCheckpointIndex(State->CheckpointIndex);
	Framework.ChangeState(&ClientState);
}

// Executes the update grid command
void _EditorState::ExecuteUpdateGridMode(_EditorState *State, ae::_Element *Element) {
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
void _EditorState::ExecuteSelectPalette(ae::_Element *Button, int ClickType) {
	if(!Button)
		return;

	// Didn't click a button
	//if(Button->TextureIndex == 0) {
	//	Brush[CurrentPalette] = nullptr;
	//	return;
	//}

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
