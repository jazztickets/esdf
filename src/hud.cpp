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
#include <hud.h>
#include <objects/object.h>
#include <objects/render.h>
#include <objects/health.h>
#include <objects/item.h>
#include <ui/label.h>
#include <ui/image.h>
#include <ui/button.h>
#include <constants.h>
#include <graphics.h>
#include <font.h>
#include <config.h>
#include <assets.h>
#include <actions.h>
#include <texture.h>
#include <files.h>
#include <sstream>
#include <iomanip>
#include <SDL_mouse.h>

// Initialize
_HUD::_HUD() :
	Player(nullptr),
	InventoryOpen(false),
	DragStart(nullptr),
	CursorItem(nullptr),
	CursorOverItem(nullptr),
	CursorSkill(-1),
	LastEntityHit(nullptr),
	CrosshairScale(0.0f),
	MessageTimer(0.0),
	MessageBoxTimer(0.0) {

}

// Shut down
_HUD::~_HUD() {
}

// Set inventory state
void _HUD::SetInventoryOpen(bool Value) {
	if(InventoryOpen == Value)
		return;

	InventoryOpen = Value;
	if(InventoryOpen) {
		Graphics.ShowCursor(CURSOR_MAIN);
	}
	else {
		Graphics.ShowCursor(CURSOR_CROSS);
		DragStart = nullptr;
		CursorItem = CursorOverItem = nullptr;
	}
}

// Handle mouse events
void _HUD::MouseEvent(const _MouseEvent &MouseEvent) {
	/*
	if(!GetInventoryOpen())
		return;

	_Element *HitElement;

	HitElement = Assets.Elements["element_hud_inventory"]->GetHitElement();
	if(MouseEvent.Button == SDL_BUTTON_LEFT) {

		// Start dragging an item
		if(MouseEvent.Pressed) {
			if(HitElement && HitElement->ID >= 0 && Player->Player->CanDropItem()) {
				DragStart = HitElement;
				CursorItem = Player->Player->Inventory[DragStart->ID];
				ClickOffset = MouseEvent.Position - HitElement->GetBounds().GetMidPoint();
			}
		}
		else {

			// Was dragging an item
			if(CursorItem) {

				// Dropped outside the inventory
				if(!HitElement) {
					Player->Player->DropItem(DragStart->ID);
				}
				else if(HitElement->ID >= 0) {
					Player->Player->SwapInventory(DragStart->ID, HitElement->ID);
				}
			}

			// Swap inventory
			CursorItem = nullptr;
			DragStart = nullptr;
		}

		//if(HitElement)
		//	printf("%d %s\n", HitElement->ID, HitElement->Identifier.c_str());
	}
	else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
		if(MouseEvent.Pressed) {
			if(HitElement && HitElement->ID >= 0) {
				Player->Player->UseMedkit(HitElement->ID);
			}
		}
	}

	HitElement = Assets.Elements["element_hud_skills"]->GetHitElement();
	if(MouseEvent.Pressed && MouseEvent.Button == SDL_BUTTON_LEFT) {
		if(HitElement && HitElement->ID >= 0) {
			Player->Player->UpdateSkill(HitElement->ID, 1);
		}
	}
	*/
}

// Update phase
void _HUD::Update(double FrameTime, float Radius) {

	LastEntityHitTimer += FrameTime;
	CursorOverItem = nullptr;
	CursorSkill = -1;

	// Update crosshair
	CrosshairScale += (Radius - CrosshairScale) / HUD_CROSSHAIRDIVISOR;
	if(CrosshairScale < HUD_MINCROSSHAIRSCALE)
		CrosshairScale = HUD_MINCROSSHAIRSCALE;
	/*
	// Update inventory
	if(GetInventoryOpen()) {
		Assets.Elements["element_hud_inventory"]->Update(FrameTime, Input.GetMouse());
		Assets.Elements["element_hud_skills"]->Update(FrameTime, Input.GetMouse());

		_Element *HitElement;
		HitElement = Assets.Elements["element_hud_inventory"]->GetHitElement();
		if(HitElement && HitElement->ID >= 0)
			CursorOverItem = Player->Player->Inventory[HitElement->ID];

		HitElement = Assets.Elements["element_hud_skills"]->GetHitElement();
		if(HitElement && HitElement->ID >= 0)
			UpdateSkillInfo(HitElement->ID, Input.GetMouse().X, Input.GetMouse().Y);
	}

	// Update health display
	if(LastEntityHit != nullptr && (LastEntityHitTimer > HUD_ENTITYHEALTHDISPLAYPERIOD || LastEntityHit->Deleted)) {
		LastEntityHit = nullptr;
	}

	if(MessageTimer > 0.0) {
		MessageTimer -= FrameTime;
	}

	if(MessageBoxTimer > 0.0) {
		MessageBoxTimer -= FrameTime;
	}*/
}

// Draw phase
void _HUD::Render() {

	// FPS
	std::ostringstream Buffer;
	Buffer << Graphics.FramesPerSecond << " FPS";
	Assets.Labels["label_hud_fps"]->Text = Buffer.str();
	Assets.Labels["label_hud_fps"]->Render();
	Buffer.str("");

	// Message
	if(MessageTimer > 0.0) {
		if(MessageTimer < 1.0)
			Assets.Labels["label_hud_message"]->Fade = MessageTimer;

		Assets.Labels["label_hud_message"]->Render();
	}

	// Message Box
	if(MessageBoxTimer > 0.0) {
		if(MessageBoxTimer < 1.0)
			Assets.Elements["element_hud_messagebox"]->Fade = MessageBoxTimer;

		Assets.Elements["element_hud_messagebox"]->Render();
	}

	// Draw enemy health
	/*if(LastEntityHit != nullptr) {
		Assets.Labels["label_hud_enemy_name"]->Text = LastEntityHit->GetName();
		Assets.Images["image_enemy_health_full"]->SetWidth(Assets.Elements["element_hud_enemy_info"]->Size.X * LastEntityHit->GetHealthPercentage());
		Assets.Elements["element_hud_enemy_info"]->Render();
	}
	*/

	// Draw player health
	if(!Player)
		return;

	if(Player->HasComponent("health")) {
		_Health *Health = (_Health *)Player->Components["health"];
		Buffer << Health->Health << "/" << Health->MaxHealth;
		Assets.Labels["label_hud_player_health"]->Text = Buffer.str();
		Buffer.str("");

		if(Health->MaxHealth > 0) {
			Assets.Images["image_player_health_full"]->SetWidth(Assets.Elements["element_hud_player_health"]->Size.x * ((float)Health->Health / Health->MaxHealth));
			Assets.Elements["element_hud_player_health"]->Render();
		}
	}

	// Draw experience bar
	Buffer << 50 << " / " << 200 << " XP";
	Assets.Labels["label_hud_experience"]->Text = Buffer.str();
	Buffer.str("");
	Assets.Images["image_experience_bar_full"]->SetWidth(Assets.Elements["element_hud_experience"]->Size.x * 0.25f);
	Assets.Elements["element_hud_experience"]->Render();

	// Draw player name and level
	Assets.Labels["label_hud_player_name"]->Text = "Jackson";
	Buffer << "Level " << 1;
	Assets.Labels["label_hud_player_level"]->Text = Buffer.str();
	Buffer.str("");
	Assets.Elements["element_hud_player_info"]->Render();
/*
	// Reload indicator
	if(Player->Player->Reloading) {
		DrawIndicator("Reload", Player->Player->GetReloadPercent(), Assets.Textures["hud/reload0.png"]);
	}
	else if(!Player->Player->HasAmmo() && !Player->Player->SwitchingWeapons && Player->Player->GetMainHand() && Player->Player->GetMainHand()->RoundSize > 0)
		DrawIndicator("Reload");

	// Weapon switch indicator
	if(Player->Player->SwitchingWeapons) {
		DrawIndicator("Switching Weapons", Player->Player->GetWeaponSwitchPercent(), Assets.Textures["hud/weaponswitch0.png"]);
	}

	// Draw weapons
	DrawHUDWeapon(Player->Player->GetMainHand(), Assets.Elements["element_hud_mainhand"], Assets.Images["image_weapon0"], Assets.Labels["label_hud_mainhand_ammo"]);
	DrawHUDWeapon(Player->Player->GetOffHand(), Assets.Elements["element_hud_offhand"], Assets.Images["image_weapon1"], Assets.Labels["label_hud_offhand_ammo"]);
	*/
	// Draw character screen
	RenderCharacterScreen();
	/*
	if(CursorOverItem && CursorItem != CursorOverItem) {
		RenderItemInfo(CursorOverItem, Input.GetMouse().X, Input.GetMouse().Y);
		if(CursorOverItem->Type == _Object::WEAPON && CursorOverItem != Player->Player->GetMainHand())
			RenderItemInfo(Player->Player->GetMainHand(), -100, Graphics.ScreenSize.y/2);
		else if(CursorOverItem->Type == _Object::ARMOR && CursorOverItem != Player->Player->GetArmor())
			RenderItemInfo(Player->Player->GetArmor(), -100, Graphics.ScreenSize.y/2);
	}*/
}

// Draws the crosshair
void _HUD::RenderCrosshair(const glm::vec2 &Position) {
	if(InventoryOpen)
		return;
	/*
	Graphics.SetColor(COLOR_WHITE);

	Graphics.SetDepthTest(false);
	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetVBO(VBO_CIRCLE);
	Graphics.DrawCircle(glm::vec3(Position, 0), CrosshairScale);

	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	Graphics.SetVBO(VBO_QUAD);
	Graphics.SetColor(COLOR_WHITE);
	Graphics.DrawSprite(glm::vec3(Position, 0), Assets.Textures["hud/crosshair0.png"], 0);

	Graphics.SetDepthTest(true);
	*/
}

// Draws a box and text
void _HUD::DrawIndicator(const std::string &String, float Percent, _Texture *Texture) {

	// Set text
	Assets.Labels["label_hud_indicator"]->Text = String;
	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.SetColor(COLOR_TGRAY);
	Graphics.DrawRectangle(Assets.Elements["element_hud_indicator"]->Bounds);

	// Set progress size
	Assets.Images["image_indicator_progress"]->Texture = Texture;
	Assets.Images["image_indicator_progress"]->SetWidth(Assets.Elements["element_hud_indicator"]->Size.x * Percent);
	Assets.Elements["element_hud_indicator"]->Render();
}

// Draw the weapons on the HUD
void _HUD::DrawHUDWeapon(const _Item *Weapon, _Element *Element, _Image *Image, _Label *Label) {
	if(!Weapon)
		return;
/*
	Image->SetTexture(Weapon->Texture);
	Image->SetColor(Weapon->Parent->Render->Color);
	if(Weapon->RoundSize) {
		std::ostringstream Buffer;
		Buffer << Weapon->Ammo << "/" << Weapon->RoundSize;
		Label->Text = Buffer.str();
	}
	else
		Label->Text = "";
*/
	Element->Render();
}

// Sets the last entity hit object
void _HUD::SetLastEntityHit(_Object *Entity) {

	LastEntityHit = Entity;
	LastEntityHitTimer = 0;
}

// Draw the inventory and character screen
void _HUD::RenderCharacterScreen() {
	if(!InventoryOpen)
		return;

	// Draw the inventory background
	Assets.Elements["element_hud_inventory"]->Render();

	/*
	// Set skill labels
	std::ostringstream Buffer;
	Buffer << Player->Player->SkillPointsRemaining;
	Assets.Labels["label_hud_skill_remaining_value"]->Text = Buffer.str();
	Buffer.str("");

	for(int i = 0; i < SKILL_COUNT; i++) {
		Buffer << Player->Player->Skills[i];
		Labels[LABEL_SKILL0 + i]->Text = Buffer.str();
		Buffer.str("");
	}

	Buffer << Player->Player->MinDamage << " - " << Player->Player->MaxDamage;
	Assets.Labels["label_hud_player_damage_value"]->Text = Buffer.str();
	Buffer.str("");

	Buffer << Player->Player->Defense;
	Assets.Labels["label_hud_player_defense_value"]->Text = Buffer.str();
	Buffer.str("");

	Buffer << Player->Player->MonsterKills;
	Assets.Labels["label_hud_player_kills_value"]->Text = Buffer.str();
	Buffer.str("");

	Assets.Elements["element_hud_skills"]->Render();

	// Draw inventory
	for(int i = INVENTORY_ARMOR; i < INVENTORY_BAGEND; i++) {
		if(Player->Player->HasInventory(i)) {
			if(Player->Player->Inventory[i] != CursorItem) {
				_Button *Button = (_Button *)Assets.Elements["element_hud_inventory"]->GetChildren()[i];
				if(Button) {
					Graphics.DrawCenteredImage(Button->GetBounds().GetMidPoint(), Player->Player->Inventory[i]->Texture, Player->Player->Inventory[i]->Parent->Color);
					if(i >= INVENTORY_BAGSTART && Player->Player->Inventory[i]->CanStack()) {
						DrawItemCount(Player->Player->Inventory[i], Button->GetBounds().End.x - 2, Button->GetBounds().End.y - 2);
					}
				}
			}
		}
	}

	// Draw cursor item
	if(CursorItem) {
		glm::ivec2 Position(Input.GetMouse() - ClickOffset);
		Graphics.DrawCenteredImage(Position, CursorItem->Texture, CursorItem->Parent->Color);
		if(CursorItem->CanStack())
			DrawItemCount(CursorItem, Position.X + 22, Position.Y + 22);
	}

	// Draw cursor skill
	if(CursorSkill != -1)
		Assets.Elements["element_hud_skill_info"]->Render();
		*/
}
/*
// Draw the item count text
void _HUD::DrawItemCount(_Item *Item, int X, int Y) {
	std::ostringstream Buffer;
	Buffer << Item->Count;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
	Buffer.str("");
}
*/
// Draw the item popup window
void _HUD::RenderItemInfo(_Item *Item, int DrawX, int DrawY) {
	/*if(!Item)
		return;

	int PadX = 8;
	int Width;
	int Height;

	// TODO cleanup
	if(Item->Type == _Object::WEAPON) {
		Width = 245;
		Height = 375;
	}
	else if(Item->Type == _Object::ARMOR) {
		Width = 220;
		Height = 170;
	}
	else {
		Width = 150;
		Height = 100;
	}

	// Get title width
	_TextBounds TextBounds;
	Assets.Fonts["hud_large"]->GetStringDimensions(Item->Name, TextBounds);
	Width = std::max(Width, TextBounds.Width) + 20;

	int MinPadding = 5;
	int MinX = 5;
	int WindowOffsetX = 20;

	// Get current equipment
	_Item *MainHand = Player->Player->GetMainHand();
	_Item *EquippedArmor = Player->Player->GetArmor();
	if((Item->Type == _Object::WEAPON && MainHand && Item != MainHand) || (Item->Type == _Object::ARMOR && EquippedArmor && Item != EquippedArmor))
		MinX += Width;

	DrawX += WindowOffsetX;
	DrawY -= Height/2;
	if(DrawX < MinX)
		DrawX = MinX;
	if(DrawY < MinPadding)
		DrawY = MinPadding;
	if(DrawX > Graphics.ScreenSize.x - MinPadding - Width)
		DrawX = Graphics.ScreenSize.x - MinPadding - Width;
	if(DrawY > Graphics.ScreenSize.y - MinPadding - Height)
		DrawY = Graphics.ScreenSize.y - MinPadding - Height;
	Graphics.DrawRectangle(DrawX, DrawY, DrawX + Width, DrawY + Height, glm::vec4(0, 0, 0, 0.8f), true);

	DrawY += 25;
	DrawX += Width/2;
	Assets.Fonts["hud_large"]->DrawText(Item->Name, DrawX, DrawY, COLOR_WHITE, CENTER_BASELINE);

	//DrawY += 16;
	//Assets.Fonts["hud_small"]->DrawText(Item->GetTypeAsString(), DrawX, DrawY, COLOR_WHITE, CENTER_BASELINE);

	DrawY += 10;
	switch(Item->Type) {
		case _Object::WEAPON: {
			std::ostringstream Buffer;
			_Item *Weapon = (_Item *)Item;
			glm::vec4 TextColor;

			// Damage
			TextColor = COLOR_WHITE;
			if(MainHand) {
				if(Weapon->GetAverageDamage() > MainHand->GetAverageDamage())
					TextColor = COLOR_GREEN;
				else if(Weapon->GetAverageDamage() < MainHand->GetAverageDamage())
					TextColor = COLOR_RED;
			}
			DrawY += 20;
			Buffer << Weapon->MinDamage << " - " << Weapon->MaxDamage;
			Assets.Fonts["hud_medium"]->DrawText("Damage", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
			Buffer.str("");

			// Clip size
			if(Weapon->RoundSize) {
				TextColor = COLOR_WHITE;
				if(MainHand) {
					if(Weapon->RoundSize > MainHand->RoundSize)
						TextColor = COLOR_GREEN;
					else if(Weapon->RoundSize < MainHand->RoundSize)
						TextColor = COLOR_RED;
				}
				DrawY += 20;
				Buffer << Weapon->Ammo << "/" << Weapon->RoundSize;
				Assets.Fonts["hud_medium"]->DrawText("Rounds", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
				Buffer.str("");
			}

			// Attacks
			if(Weapon->BulletsShot > 1) {
				TextColor = COLOR_WHITE;
				if(MainHand) {
					if(Weapon->BulletsShot > MainHand->BulletsShot)
						TextColor = COLOR_GREEN;
					else if(Weapon->BulletsShot < MainHand->BulletsShot)
						TextColor = COLOR_RED;
				}

				DrawY += 20;
				Buffer << Weapon->BulletsShot;
				std::string AttackCountText;
				if(Weapon->Stats.Type == WEAPON_MELEE)
					AttackCountText = "Attacks/Swing";
				else
					AttackCountText = "Bullets/Shot";
				Assets.Fonts["hud_medium"]->DrawText(AttackCountText, DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
				Buffer.str("");
			}

			// Fire rate
			if(Weapon->FirePeriod) {
				TextColor = COLOR_WHITE;
				if(MainHand) {
					if(Weapon->FirePeriod < MainHand->FirePeriod)
						TextColor = COLOR_GREEN;
					else if(Weapon->FirePeriod > MainHand->FirePeriod)
						TextColor = COLOR_RED;
				}

				DrawY += 20;
				Buffer << std::setprecision(3) << 1 / Weapon->FirePeriod << "/s";
				std::string AttackCountText;
				if(Weapon->Stats.Type == WEAPON_MELEE)
					AttackCountText = "Attack Rate";
				else
					AttackCountText = "Fire Rate";
				Assets.Fonts["hud_medium"]->DrawText(AttackCountText, DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
				Buffer.str("");
				Buffer << std::setprecision(6);
			}

			// Weapon Spread
			TextColor = COLOR_WHITE;
			if(MainHand && MainHand->IsMelee() == Weapon->IsMelee()) {
				if(Weapon->GetAverageAccuracy() < MainHand->GetAverageAccuracy()) {

					// Less is worse for melee
					if(Weapon->Stats.Type == WEAPON_MELEE)
						TextColor = COLOR_RED;
					else
						TextColor = COLOR_GREEN;
				}
				else if(Weapon->GetAverageAccuracy() > MainHand->GetAverageAccuracy()) {

					// Bigger is better for melee
					if(Weapon->Stats.Type == WEAPON_MELEE)
						TextColor = COLOR_GREEN;
					else
						TextColor = COLOR_RED;
				}
			}
			DrawY += 20;
			if(Weapon->Stats.Type == WEAPON_MELEE) {
				Buffer << Weapon->MaxAccuracy << " degrees";
				Assets.Fonts["hud_medium"]->DrawText("Swing Arc", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			}
			else {
				Buffer << (int)(Weapon->MinAccuracy + 0.5f) << " - " << (int)(Weapon->MaxAccuracy + 0.5f);
				Assets.Fonts["hud_medium"]->DrawText("Spread", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			}
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
			Buffer.str("");

			// Reload speed
			if(Weapon->ReloadPeriod > 1) {
				TextColor = COLOR_WHITE;
				if(MainHand) {
					if(Weapon->ReloadPeriod < MainHand->ReloadPeriod)
						TextColor = COLOR_GREEN;
					else if(Weapon->ReloadPeriod > MainHand->ReloadPeriod)
						TextColor = COLOR_RED;
				}

				DrawY += 20;
				Buffer << Weapon->ReloadPeriod << "s";
				std::string AttackCountText;
				Assets.Fonts["hud_medium"]->DrawText("Reload Time", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
				Buffer.str("");
			}

			// Ammo type
			if(Weapon->AmmoType) {
				DrawY += 20;
				Buffer << _Ammo::ToString(Weapon->AmmoType);
				Assets.Fonts["hud_medium"]->DrawText("Ammo Type", DrawX  - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY);
				Buffer.str("");
			}

			// Components
			if(Weapon->MaxComponents >= 1) {
				TextColor = COLOR_WHITE;
				if(MainHand) {
					if(Weapon->MaxComponents > MainHand->MaxComponents)
						TextColor = COLOR_GREEN;
					else if(Weapon->MaxComponents < MainHand->MaxComponents)
						TextColor = COLOR_RED;
				}

				DrawY += 20;
				Buffer << Weapon->GetComponents() << "/" << Weapon->MaxComponents;
				Assets.Fonts["hud_medium"]->DrawText("Components", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
				Buffer.str("");
			}

			// Bonuses
			TextColor = COLOR_WHITE;
			bool First = true;
			for(int i = 0; i < UPGRADE_TYPES; i++) {
				if(Weapon->Bonus[i]) {
					if(First)
						DrawY += 10;
					DrawY += 20;
					Buffer << "+" << Weapon->Bonus[i] * 100.0f << "% " << _Upgrade::ToString(i, Weapon->GetWeaponType());
					Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX, DrawY, TextColor, CENTER_BASELINE);
					Buffer.str("");

					First = false;
				}
			}
		} break;
		case _Object::ARMOR: {
			_Item *Armor = (_Item *)Item;
			std::ostringstream Buffer;
			glm::vec4 TextColor;

			DrawX += 40;

			// Strength required
			TextColor = COLOR_WHITE;
			if(EquippedArmor) {
				if(Armor->StrengthRequirement < EquippedArmor->StrengthRequirement)
					TextColor = COLOR_GREEN;
				else if(Armor->StrengthRequirement > EquippedArmor->StrengthRequirement)
					TextColor = COLOR_RED;
			}
			DrawY += 20;
			Buffer << Armor->StrengthRequirement;
			Assets.Fonts["hud_medium"]->DrawText("Strength Required", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
			Buffer.str("");

			// Defense
			TextColor = COLOR_WHITE;
			if(EquippedArmor) {
				if(Armor->Defense > EquippedArmor->Defense)
					TextColor = COLOR_GREEN;
				else if(Armor->Defense < EquippedArmor->Defense)
					TextColor = COLOR_RED;
			}

			DrawY += 20;
			Buffer << Armor->Defense;
			Assets.Fonts["hud_medium"]->DrawText("Defense", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
			Buffer.str("");
		} break;
		case _Object::MISCITEM: {
			_Item *MiscItem = (_Item *)Item;
			if(MiscItem->MiscItemType == MISCITEM_MEDKIT) {
				std::ostringstream Buffer;

				// Heal amount
				DrawY += 20;
				Buffer << "+" << Player->GetMedkitHealAmount(MiscItem->GetLevel()) << " HP";
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX, DrawY, COLOR_GREEN, CENTER_BASELINE);
			}
		} break;

		case _Object::UPGRADE: {

			_Item *Upgrade = (_Item *)Item;
			std::ostringstream Buffer;
			// Bonus
			DrawY += 20;
			if(Upgrade->UpgradeType == UPGRADE_ATTACKS)
				Buffer << "+" << (int)(Upgrade->Bonus) << " Attack Count";
			else
				Buffer << "+" << (int)(Upgrade->Bonus * 100.0f + 0.5f) << "% " << _Upgrade::ToString(Upgrade->UpgradeType, -1);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawX, DrawY, COLOR_WHITE, CENTER_BASELINE);

		} break;

	}
	*/
}

// Draw the skill popup window
void _HUD::UpdateSkillInfo(int Skill, int DrawX, int DrawY) {
	/*
	CursorSkill = Skill;

	DrawX -= Assets.Elements["element_hud_skill_info"]->Size.X + 15;
	DrawY -= Assets.Elements["element_hud_skill_info"]->Size.Y + 15;
	if(DrawX < 10)
		DrawX = 10;
	if(DrawY < 10)
		DrawY = 10;

	// Move window
	Assets.Elements["element_hud_skill_info"]->SetOffset(glm::ivec2(DrawX, DrawY));

	// Get skill description
	std::ostringstream Buffer, BufferNext;
	Buffer << std::setprecision(3);
	BufferNext << std::setprecision(3);
	switch(Skill) {
		case SKILL_STRENGTH:
			Assets.Labels["label_hud_skill"]->Text = "Allows you to equip heavier armor";
			Buffer << "+" << Assets.GetSkill(Player->Player->Skills[Skill], Skill) << " Strength";
			BufferNext << "+" << Assets.GetSkill(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << " Strength";
		break;
		case SKILL_HEALTH:
			Assets.Labels["label_hud_skill"]->Text = "Increases health";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Health";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Health";
		break;
		case SKILL_ACCURACY:
			Assets.Labels["label_hud_skill"]->Text = "Increases gun accuracy";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Accuracy";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Accuracy";
		break;
		case SKILL_RELOADSPEED:
			Assets.Labels["label_hud_skill"]->Text = "Increases reload speed";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Reload Speed";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Reload Speed";
		break;
		case SKILL_ATTACKSPEED:
			Assets.Labels["label_hud_skill"]->Text = "Increases attack speed";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Attack Speed";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Attack Speed";
		break;
		case SKILL_MOVESPEED:
			Assets.Labels["label_hud_skill"]->Text = "Increases move speed";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Move Speed";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Move Speed";
		break;
		case SKILL_STAT:
			Assets.Labels["label_hud_skill"]->Text = "Stat";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Stat";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Stat";
		break;
		case SKILL_MAXINVENTORY:
			Assets.Labels["label_hud_skill"]->Text = "Increases max inventory stack size";
			Buffer << "+" << Assets.GetSkill(Player->Player->Skills[Skill], Skill) << " Stacks";
			BufferNext << "+" << Assets.GetSkill(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << " Stacks";
		break;
	}

	// Wrap text
	Assets.Labels["label_hud_skill"]->SetWrap(Assets.Elements["element_hud_skill_info"]->Size.X - 20);

	Assets.Labels["label_hud_skill_level"]->Text = Buffer.str();
	if(Player->Player->Skills[Skill]+1 > GAME_SKILLLEVELS)
		BufferNext.str("");
	Assets.Labels["label_hud_skill_level_next"]->Text = BufferNext.str();
	*/
}

// Draw death message
void _HUD::RenderDeathScreen() {
	Assets.Fonts["hud_largest"]->DrawText("You Died!", glm::vec2(Graphics.WindowSize.x / 2, Graphics.WindowSize.y / 2 - 200), COLOR_WHITE, CENTER_MIDDLE);
	Assets.Fonts["hud_large"]->DrawText(std::string("Press [") + Actions.GetInputNameForAction(_Actions::USE) + "] to continue", glm::vec2(Graphics.WindowSize.x / 2, Graphics.WindowSize.y / 2 - 150), COLOR_WHITE, CENTER_MIDDLE);
}

// Show hud message
void _HUD::ShowTextMessage(const std::string &Message, double Time) {
	Assets.Labels["label_hud_message"]->Text = Message;
	Assets.Labels["label_hud_message"]->Fade = 1.0f;
	MessageTimer = Time;
}

// Show message box
void _HUD::ShowMessageBox(const std::string &Message, double Time) {
	if(MessageBoxTimer > 0.0 && Assets.Labels["label_hud_messagebox"]->Text == Message)
		return;

	Assets.Labels["label_hud_messagebox"]->Text = Message;
	Assets.Labels["label_hud_messagebox"]->SetWrap(Assets.Elements["element_hud_messagebox"]->Size.x - 25);

	Assets.Elements["element_hud_messagebox"]->Fade = 1.0f;
	MessageBoxTimer = Time;
}

