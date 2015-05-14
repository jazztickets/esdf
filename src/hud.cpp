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
_HUD::_HUD() {
	Player = nullptr;

	LastEntityHit = nullptr;
	DragStart = nullptr;
	CursorItem = CursorOverItem = nullptr;
	CursorSkill = -1;
	CrosshairScale = 0.0f;
	MessageTimer = 0.0;
	MessageBoxTimer = 0.0;
	InventoryOpen = false;

	// Get textures
	Fonts[FONT_TINY] = Assets.Fonts["hud_tiny"];
	Fonts[FONT_SMALL] = Assets.Fonts["hud_small"];
	Fonts[FONT_MEDIUM] = Assets.Fonts["hud_medium"];
	Fonts[FONT_LARGE] = Assets.Fonts["hud_large"];
	Fonts[FONT_LARGER] = Assets.Fonts["hud_larger"];
	Fonts[FONT_LARGEST] = Assets.Fonts["hud_largest"];
	CrosshairID = Assets.Textures["hud/crosshair0.png"];
	ReloadTexture = Assets.Textures["hud/reload0.png"];
	WeaponSwitchTexture = Assets.Textures["hud/weaponswitch0.png"];

	// Elements
	Labels[LABEL_FPS] = Assets.Labels["label_hud_fps"];
	Labels[LABEL_MESSAGE] = Assets.Labels["label_hud_message"];
	Labels[LABEL_MESSAGEBOX] = Assets.Labels["label_label_hud_messagebox"];

	Elements[ELEMENT_PLAYERINFO] = Assets.GetElement("element_hud_player_info");
	Labels[LABEL_PLAYERNAME] = Assets.Labels["label_hud_player_name"];
	Labels[LABEL_PLAYERLEVEL] = Assets.Labels["label_hud_player_level"];
	Labels[LABEL_PLAYERHEALTH] = Assets.Labels["hud_player_health"];

	Elements[ELEMENT_ENEMYINFO] = Assets.GetElement("element_hud_enemy_info");
	Labels[LABEL_ENEMYNAME] = Assets.Labels["label_hud_enemy_name"];

	Elements[ELEMENT_PLAYERHEALTH] = Assets.GetElement("element_hud_player_health");
	Images[IMAGE_PLAYERHEALTH] = Assets.GetImage("image_player_health_full");
	Labels[LABEL_PLAYERHEALTH] = Assets.Labels["label_hud_player_health"];

	Images[IMAGE_ENEMYHEALTH] = Assets.GetImage("image_enemy_health_full");

	Elements[ELEMENT_INDICATOR] = Assets.GetElement("element_hud_indicator");
	Images[IMAGE_RELOAD] = Assets.GetImage("image_indicator_progress");
	Labels[LABEL_INDICATOR] = Assets.Labels["label_hud_indicator"];

	Elements[ELEMENT_EXPERIENCE] = Assets.GetElement("element_hud_experience");
	Images[IMAGE_EXPERIENCE] = Assets.GetImage("image_experience_bar_full");
	Labels[LABEL_EXPERIENCE] = Assets.Labels["label_hud_experience"];

	Elements[ELEMENT_MAINHAND] = Assets.GetElement("element_hud_mainhand");
	Images[IMAGE_MAINHAND_ICON] = Assets.GetImage("image_weapon0");
	Labels[LABEL_MAINHAND_AMMO] = Assets.Labels["label_hud_mainhand_ammo"];

	Elements[ELEMENT_OFFHAND] = Assets.GetElement("element_hud_offhand");
	Images[IMAGE_OFFHAND_ICON] = Assets.GetImage("image_weapon1");
	Labels[LABEL_OFFHAND_AMMO] = Assets.Labels["label_hud_offhand_ammo"];

	Elements[ELEMENT_INVENTORY] = Assets.GetElement("element_hud_inventory");
	Elements[ELEMENT_SKILLS] = Assets.GetElement("element_hud_skills");
	Labels[LABEL_SKILL_REMAINING] = Assets.Labels["label_hud_skill_remaining_value"];
	Labels[LABEL_SKILL0] = Assets.Labels["label_hud_skill0_value"];
	Labels[LABEL_SKILL1] = Assets.Labels["label_hud_skill1_value"];
	Labels[LABEL_SKILL2] = Assets.Labels["label_hud_skill2_value"];
	Labels[LABEL_SKILL3] = Assets.Labels["label_hud_skill3_value"];
	Labels[LABEL_SKILL4] = Assets.Labels["label_hud_skill4_value"];
	Labels[LABEL_SKILL5] = Assets.Labels["label_hud_skill5_value"];
	Labels[LABEL_SKILL6] = Assets.Labels["label_hud_skill6_value"];
	Labels[LABEL_SKILL7] = Assets.Labels["label_hud_skill7_value"];

	Labels[LABEL_DAMAGE] = Assets.Labels["label_hud_player_damage_value"];
	Labels[LABEL_DEFENSE] = Assets.Labels["label_hud_player_defense_value"];
	Labels[LABEL_KILLS] = Assets.Labels["label_hud_player_kills_value"];

	Elements[ELEMENT_SKILLINFO] = Assets.GetElement("element_hud_skill_info");
	Labels[LABEL_SKILLTEXT] = Assets.Labels["label_hud_skill"];
	Labels[LABEL_SKILL_LEVEL] = Assets.Labels["label_hud_skill_level"];
	Labels[LABEL_SKILL_LEVEL_NEXT] = Assets.Labels["label_hud_skill_level_next"];

	Elements[ELEMENT_MESSAGE] = Assets.GetElement("element_hud_messagebox");
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

	}
	else {
		DragStart = nullptr;
		CursorItem = CursorOverItem = nullptr;
	}

	Graphics.ShowCursor(InventoryOpen);
}

// Handle mouse events
void _HUD::MouseEvent(const _MouseEvent &MouseEvent) {
	/*
	if(!GetInventoryOpen())
		return;

	_Element *HitElement;

	HitElement = Elements[ELEMENT_INVENTORY]->GetHitElement();
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

	HitElement = Elements[ELEMENT_SKILLS]->GetHitElement();
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
		Elements[ELEMENT_INVENTORY]->Update(FrameTime, Input.GetMouse());
		Elements[ELEMENT_SKILLS]->Update(FrameTime, Input.GetMouse());

		_Element *HitElement;
		HitElement = Elements[ELEMENT_INVENTORY]->GetHitElement();
		if(HitElement && HitElement->ID >= 0)
			CursorOverItem = Player->Player->Inventory[HitElement->ID];

		HitElement = Elements[ELEMENT_SKILLS]->GetHitElement();
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
	Labels[LABEL_FPS]->Text = Buffer.str();
	Labels[LABEL_FPS]->Render();
	Buffer.str("");

	// Message
	if(MessageTimer > 0.0) {
		if(MessageTimer < 1.0)
			Labels[LABEL_MESSAGE]->Fade = MessageTimer;

		Labels[LABEL_MESSAGE]->Render();
	}

	// Message Box
	if(MessageBoxTimer > 0.0) {
		if(MessageBoxTimer < 1.0)
			Elements[ELEMENT_MESSAGE]->Fade = MessageBoxTimer;

		Elements[ELEMENT_MESSAGE]->Render();
	}

	// Draw enemy health
	/*if(LastEntityHit != nullptr) {
		Labels[LABEL_ENEMYNAME]->Text = LastEntityHit->GetName();
		Images[IMAGE_ENEMYHEALTH]->SetWidth(Elements[ELEMENT_ENEMYINFO]->Size.X * LastEntityHit->GetHealthPercentage());
		Elements[ELEMENT_ENEMYINFO]->Render();
	}
	*/

	// Draw player health
	Buffer << 50 << "/" << 100;
	Labels[LABEL_PLAYERHEALTH]->Text = Buffer.str();
	Buffer.str("");

	Images[IMAGE_PLAYERHEALTH]->SetWidth(Elements[ELEMENT_PLAYERHEALTH]->Size.x * 0.5f);
	Elements[ELEMENT_PLAYERHEALTH]->Render();

	// Draw experience bar
	Buffer << 50 << " / " << 200 << " XP";
	Labels[LABEL_EXPERIENCE]->Text = Buffer.str();
	Buffer.str("");
	Images[IMAGE_EXPERIENCE]->SetWidth(Elements[ELEMENT_EXPERIENCE]->Size.x * 0.25f);
	Elements[ELEMENT_EXPERIENCE]->Render();

	// Draw player name and level
	Labels[LABEL_PLAYERNAME]->Text = "Jackson";
	Buffer << "Level " << 1;
	Labels[LABEL_PLAYERLEVEL]->Text = Buffer.str();
	Buffer.str("");
	Elements[ELEMENT_PLAYERINFO]->Render();
/*
	// Reload indicator
	if(Player->Player->Reloading) {
		DrawIndicator("Reload", Player->Player->GetReloadPercent(), ReloadTexture);
	}
	else if(!Player->Player->HasAmmo() && !Player->Player->SwitchingWeapons && Player->Player->GetMainHand() && Player->Player->GetMainHand()->RoundSize > 0)
		DrawIndicator("Reload");

	// Weapon switch indicator
	if(Player->Player->SwitchingWeapons) {
		DrawIndicator("Switching Weapons", Player->Player->GetWeaponSwitchPercent(), WeaponSwitchTexture);
	}

	// Draw weapons
	DrawHUDWeapon(Player->Player->GetMainHand(), Elements[ELEMENT_MAINHAND], Images[IMAGE_MAINHAND_ICON], Labels[LABEL_MAINHAND_AMMO]);
	DrawHUDWeapon(Player->Player->GetOffHand(), Elements[ELEMENT_OFFHAND], Images[IMAGE_OFFHAND_ICON], Labels[LABEL_OFFHAND_AMMO]);
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

	Graphics.SetDepthTest(false);
	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetVBO(VBO_CIRCLE);
	Graphics.DrawCircle(glm::vec3(Position, 0), CrosshairScale, COLOR_WHITE);

	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	Graphics.SetVBO(VBO_QUAD);
	Graphics.DrawSprite(glm::vec3(Position, 0), CrosshairID, COLOR_WHITE, 0);

	Graphics.SetDepthTest(true);
}

// Draws a box and text
void _HUD::DrawIndicator(const std::string &String, float Percent, _Texture *Texture) {

	// Set text
	Labels[LABEL_INDICATOR]->Text = String;
	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.DrawRectangle(Elements[ELEMENT_INDICATOR]->Bounds, COLOR_TGRAY);

	// Set progress size
	Images[IMAGE_RELOAD]->Texture = Texture;
	Images[IMAGE_RELOAD]->SetWidth(Elements[ELEMENT_INDICATOR]->Size.x * Percent);
	Elements[ELEMENT_INDICATOR]->Render();
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
	Elements[ELEMENT_INVENTORY]->Render();

	/*
	// Set skill labels
	std::ostringstream Buffer;
	Buffer << Player->Player->SkillPointsRemaining;
	Labels[LABEL_SKILL_REMAINING]->Text = Buffer.str();
	Buffer.str("");

	for(int i = 0; i < SKILL_COUNT; i++) {
		Buffer << Player->Player->Skills[i];
		Labels[LABEL_SKILL0 + i]->Text = Buffer.str();
		Buffer.str("");
	}

	Buffer << Player->Player->MinDamage << " - " << Player->Player->MaxDamage;
	Labels[LABEL_DAMAGE]->Text = Buffer.str();
	Buffer.str("");

	Buffer << Player->Player->Defense;
	Labels[LABEL_DEFENSE]->Text = Buffer.str();
	Buffer.str("");

	Buffer << Player->Player->MonsterKills;
	Labels[LABEL_KILLS]->Text = Buffer.str();
	Buffer.str("");

	Elements[ELEMENT_SKILLS]->Render();

	// Draw inventory
	for(int i = INVENTORY_ARMOR; i < INVENTORY_BAGEND; i++) {
		if(Player->Player->HasInventory(i)) {
			if(Player->Player->Inventory[i] != CursorItem) {
				_Button *Button = (_Button *)Elements[ELEMENT_INVENTORY]->GetChildren()[i];
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
		Elements[ELEMENT_SKILLINFO]->Render();
		*/
}
/*
// Draw the item count text
void _HUD::DrawItemCount(_Item *Item, int X, int Y) {
	std::ostringstream Buffer;
	Buffer << Item->Count;
	Fonts[FONT_TINY]->DrawText(Buffer.str(), glm::vec2(X, Y), COLOR_WHITE, RIGHT_BASELINE);
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
	Fonts[FONT_LARGE]->GetStringDimensions(Item->Name, TextBounds);
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
	Fonts[FONT_LARGE]->DrawText(Item->Name, DrawX, DrawY, COLOR_WHITE, CENTER_BASELINE);

	//DrawY += 16;
	//Fonts[FONT_SMALL]->DrawText(Item->GetTypeAsString(), DrawX, DrawY, COLOR_WHITE, CENTER_BASELINE);

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
			Fonts[FONT_MEDIUM]->DrawText("Damage", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
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
				Fonts[FONT_MEDIUM]->DrawText("Rounds", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
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
				Fonts[FONT_MEDIUM]->DrawText(AttackCountText, DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
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
				Fonts[FONT_MEDIUM]->DrawText(AttackCountText, DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
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
				Fonts[FONT_MEDIUM]->DrawText("Swing Arc", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			}
			else {
				Buffer << (int)(Weapon->MinAccuracy + 0.5f) << " - " << (int)(Weapon->MaxAccuracy + 0.5f);
				Fonts[FONT_MEDIUM]->DrawText("Spread", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			}
			Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
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
				Fonts[FONT_MEDIUM]->DrawText("Reload Time", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
				Buffer.str("");
			}

			// Ammo type
			if(Weapon->AmmoType) {
				DrawY += 20;
				Buffer << _Ammo::ToString(Weapon->AmmoType);
				Fonts[FONT_MEDIUM]->DrawText("Ammo Type", DrawX  - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY);
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
				Fonts[FONT_MEDIUM]->DrawText("Components", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
				Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
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
					Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX, DrawY, TextColor, CENTER_BASELINE);
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
			Fonts[FONT_MEDIUM]->DrawText("Strength Required", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
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
			Fonts[FONT_MEDIUM]->DrawText("Defense", DrawX - PadX, DrawY, COLOR_WHITE, RIGHT_BASELINE);
			Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX + PadX, DrawY, TextColor, LEFT_BASELINE);
			Buffer.str("");
		} break;
		case _Object::MISCITEM: {
			_Item *MiscItem = (_Item *)Item;
			if(MiscItem->MiscItemType == MISCITEM_MEDKIT) {
				std::ostringstream Buffer;

				// Heal amount
				DrawY += 20;
				Buffer << "+" << Player->GetMedkitHealAmount(MiscItem->GetLevel()) << " HP";
				Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX, DrawY, COLOR_GREEN, CENTER_BASELINE);
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
			Fonts[FONT_MEDIUM]->DrawText(Buffer.str(), DrawX, DrawY, COLOR_WHITE, CENTER_BASELINE);

		} break;

	}
	*/
}

// Draw the skill popup window
void _HUD::UpdateSkillInfo(int Skill, int DrawX, int DrawY) {
	/*
	CursorSkill = Skill;

	DrawX -= Elements[ELEMENT_SKILLINFO]->Size.X + 15;
	DrawY -= Elements[ELEMENT_SKILLINFO]->Size.Y + 15;
	if(DrawX < 10)
		DrawX = 10;
	if(DrawY < 10)
		DrawY = 10;

	// Move window
	Elements[ELEMENT_SKILLINFO]->SetOffset(glm::ivec2(DrawX, DrawY));

	// Get skill description
	std::ostringstream Buffer, BufferNext;
	Buffer << std::setprecision(3);
	BufferNext << std::setprecision(3);
	switch(Skill) {
		case SKILL_STRENGTH:
			Labels[LABEL_SKILLTEXT]->Text = "Allows you to equip heavier armor";
			Buffer << "+" << Assets.GetSkill(Player->Player->Skills[Skill], Skill) << " Strength";
			BufferNext << "+" << Assets.GetSkill(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << " Strength";
		break;
		case SKILL_HEALTH:
			Labels[LABEL_SKILLTEXT]->Text = "Increases health";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Health";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Health";
		break;
		case SKILL_ACCURACY:
			Labels[LABEL_SKILLTEXT]->Text = "Increases gun accuracy";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Accuracy";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Accuracy";
		break;
		case SKILL_RELOADSPEED:
			Labels[LABEL_SKILLTEXT]->Text = "Increases reload speed";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Reload Speed";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Reload Speed";
		break;
		case SKILL_ATTACKSPEED:
			Labels[LABEL_SKILLTEXT]->Text = "Increases attack speed";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Attack Speed";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Attack Speed";
		break;
		case SKILL_MOVESPEED:
			Labels[LABEL_SKILLTEXT]->Text = "Increases move speed";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Move Speed";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Move Speed";
		break;
		case SKILL_STAT:
			Labels[LABEL_SKILLTEXT]->Text = "Stat";
			Buffer << "+" << Assets.GetSkillPercentImprovement(Player->Player->Skills[Skill], Skill) << "% Stat";
			BufferNext << "+" << Assets.GetSkillPercentImprovement(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << "% Stat";
		break;
		case SKILL_MAXINVENTORY:
			Labels[LABEL_SKILLTEXT]->Text = "Increases max inventory stack size";
			Buffer << "+" << Assets.GetSkill(Player->Player->Skills[Skill], Skill) << " Stacks";
			BufferNext << "+" << Assets.GetSkill(Assets.GetValidSkill(Player->Player->Skills[Skill]+1), Skill) << " Stacks";
		break;
	}

	// Wrap text
	Labels[LABEL_SKILLTEXT]->SetWrap(Elements[ELEMENT_SKILLINFO]->Size.X - 20);

	Labels[LABEL_SKILL_LEVEL]->Text = Buffer.str();
	if(Player->Player->Skills[Skill]+1 > GAME_SKILLLEVELS)
		BufferNext.str("");
	Labels[LABEL_SKILL_LEVEL_NEXT]->Text = BufferNext.str();
	*/
}

// Draw death message
void _HUD::RenderDeathScreen() {
	Fonts[FONT_LARGEST]->DrawText("You Died!", glm::vec2(Graphics.WindowSize.x / 2, Graphics.WindowSize.y / 2 - 200), COLOR_WHITE, CENTER_MIDDLE);
	Fonts[FONT_LARGE]->DrawText(std::string("Press [") + Actions.GetInputNameForAction(_Actions::USE) + "] to continue", glm::vec2(Graphics.WindowSize.x / 2, Graphics.WindowSize.y / 2 - 150), COLOR_WHITE, CENTER_MIDDLE);
}

// Show hud message
void _HUD::ShowTextMessage(const std::string &Message, double Time) {
	Labels[LABEL_MESSAGE]->Text = Message;
	Labels[LABEL_MESSAGE]->Fade = 1.0f;
	MessageTimer = Time;
}

// Show message box
void _HUD::ShowMessageBox(const std::string &Message, double Time) {
	if(MessageBoxTimer > 0.0 && Labels[LABEL_MESSAGEBOX]->Text == Message)
		return;

	Labels[LABEL_MESSAGEBOX]->Text = Message;
	Labels[LABEL_MESSAGEBOX]->SetWrap(Elements[ELEMENT_MESSAGE]->Size.x - 25);

	Elements[ELEMENT_MESSAGE]->Fade = 1.0f;
	MessageBoxTimer = Time;
}