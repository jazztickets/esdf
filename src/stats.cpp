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
#include <stats.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/controller.h>
#include <objects/animation.h>
#include <objects/render.h>
#include <objects/shape.h>
#include <objects/zone.h>
#include <objects/shot.h>
#include <objects/health.h>
#include <objects/ai.h>
#include <ae/assets.h>
#include <ae/utils.h>
#include <constants.h>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <iostream>

static std::vector<std::string> Components = {
	"physics",
	"controller",
	"animation",
	"render",
	"shape",
	"zone",
	"shot",
	"health",
	"ai",
};

// Constructor
_Stats::_Stats() {

	// Load components
	for(const auto &Iterator : Components)
		LoadComponent(Iterator, STATS_BASEPATH + Iterator + ".tsv");

	// Load objects
	LoadObjects(STATS_OBJECTS);

	// Clear stats map
	ComponentStats.clear();
}

// Destructor
_Stats::~_Stats() {
}

// Object factory
_Object *_Stats::CreateObject(const std::string Identifier, bool IsServer) const {
	const auto &Iterator = Objects.find(Identifier);
	if(Iterator == Objects.end())
		return nullptr;

	const _ObjectStat &ObjectStat = Iterator->second;

	// Create object
	_Object *Object = new _Object();
	Object->Identifier = Identifier;
	Object->Name = ObjectStat.Name;
	Object->Lifetime = ObjectStat.Lifetime;
	if(Object->Lifetime == 0.0f)
		Object->Event = 1;
	Object->Server = IsServer;

	// Create physics
	const auto &ComponentIterator = ObjectStat.Components.find("physics");
	if(ComponentIterator != ObjectStat.Components.end()) {
		Object->Physics = new _Physics(Object, (const _PhysicsStat *)ComponentIterator->second.get());
		if(IsServer)
			Object->Physics->RenderDelay = false;

		Object->Components["physics"] = Object->Physics;
	}

	// Create controller
	{
		const auto &ComponentIterator = ObjectStat.Components.find("controller");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Components["controller"] = new _Controller(Object, (const _ControllerStat *)ComponentIterator->second.get());
		}
	}

	// Create animation
	{
		const auto &ComponentIterator = ObjectStat.Components.find("animation");
		if(ComponentIterator != ObjectStat.Components.end()) {
			const _AnimationStat *AnimationStat = (const _AnimationStat *)ComponentIterator->second.get();
			Object->Animation = new _Animation(Object);

			// Load animation templates
			for(const auto &Template : AnimationStat->Templates)
				Object->Animation->Templates.push_back(Assets.AnimationTemplates[Template]);

			// Set default frame
			Object->Animation->Stop();
			Object->Animation->FramePeriod = 0.07;
			Object->Components["animation"] = Object->Animation;
		}
	}

	// Create render
	{
		const auto &ComponentIterator = ObjectStat.Components.find("render");
		if(ComponentIterator != ObjectStat.Components.end()) {
			const _RenderStat *RenderStat = (const _RenderStat *)ComponentIterator->second.get();
			Object->Render = new _Render(Object, RenderStat);
			Object->Render->Color = Assets.Colors[RenderStat->ColorIdentifier];
			Object->Render->Program = Assets.Programs[RenderStat->ProgramIdentifier];
			Object->Render->Texture = Assets.Textures[RenderStat->TextureIdentifier];
			Object->Render->Mesh = Assets.Meshes[RenderStat->MeshIdentifier];
			Object->Render->Debug = _Render::DEBUG_ALL;
			Object->Components["render"] = Object->Render;
		}
	}

	// Create shape
	{
		const auto &ComponentIterator = ObjectStat.Components.find("shape");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Shape = new _Shape(Object, (const _ShapeStat *)ComponentIterator->second.get());
			Object->Components["shape"] = Object->Shape;
		}
	}

	// Create zone
	{
		const auto &ComponentIterator = ObjectStat.Components.find("zone");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Components["zone"] = new _Zone(Object, (const _ZoneStat *)ComponentIterator->second.get());
		}
	}

	// Create shot
	{
		const auto &ComponentIterator = ObjectStat.Components.find("shot");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Components["shot"] = new _Shot(Object, (const _ShotStat *)ComponentIterator->second.get());
		}
	}

	// Create health
	{
		const auto &ComponentIterator = ObjectStat.Components.find("health");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Components["health"] = new _Health(Object, (const _HealthStat *)ComponentIterator->second.get());
		}
	}

	// Create ai
	{
		const auto &ComponentIterator = ObjectStat.Components.find("ai");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Components["ai"] = new _Ai(Object, (const _AiStat *)ComponentIterator->second.get());
		}
	}

	return Object;
}

// Load object stats
void _Stats::LoadObjects(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_ObjectStat ObjectStat;
		GetTSVToken(File, ObjectStat.Identifier);
		GetTSVToken(File, ObjectStat.Name);

		// Load components
		for(auto &ComponentType : Components) {
			std::string ComponentIdentifier;
			GetTSVToken(File, ComponentIdentifier);
			if(ComponentIdentifier != "") {
				if(ComponentStats[ComponentType].find(ComponentIdentifier) == ComponentStats[ComponentType].end())
					throw std::runtime_error("Cannot find '" + ComponentType + "' component: " + ComponentIdentifier);

				ObjectStat.Components[ComponentType] = ComponentStats[ComponentType][ComponentIdentifier];
				ComponentIdentifier.clear();
			}
		}

		// Check for duplicates
		if(ObjectStat.Identifier != "" && Objects.find(ObjectStat.Identifier) != Objects.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ObjectStat.Identifier);

		// Get misc stats
		File >> ObjectStat.Lifetime;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Add row
		Objects[ObjectStat.Identifier] = ObjectStat;
	}

	// Close file
	File.close();
}

// Load a component tsv file into the ComponentStats map
void _Stats::LoadComponent(const std::string &Type, const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_Stat> Stat = LoadComponentType(Type, File);

		// Check for duplicates
		if(ComponentStats[Type].find(Stat->Identifier) != ComponentStats[Type].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + Stat->Identifier);

		// Add row
		ComponentStats[Type][Stat->Identifier] = Stat;
	}

	// Close file
	File.close();
}

// Read a component line from a file and return the new stat object
std::shared_ptr<_Stat> _Stats::LoadComponentType(const std::string &Type, std::ifstream &File) {

	if(Type == "physics") {
		std::shared_ptr<_PhysicsStat> Stat(new _PhysicsStat());
		GetTSVToken(File, Stat->Identifier);

		File >> Stat->CollisionResponse;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		return Stat;
	}
	else if(Type == "controller") {
		std::shared_ptr<_ControllerStat> Stat(new _ControllerStat());
		GetTSVToken(File, Stat->Identifier);

		File >> Stat->Speed;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		return Stat;
	}
	else if(Type == "animation") {
		std::shared_ptr<_AnimationStat> Stat(new _AnimationStat());
		GetTSVToken(File, Stat->Identifier);

		// Read animation templates
		bool EndOfLine = false;
		while(!EndOfLine && File.peek() != EOF) {
			std::string Token;
			GetTSVToken(File, Token, &EndOfLine);

			if(Token != "")
				Stat->Templates.push_back(Token);
		}

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		return Stat;
	}
	else if(Type == "render") {
		std::shared_ptr<_RenderStat> Stat(new _RenderStat());
		GetTSVToken(File, Stat->Identifier);
		GetTSVToken(File, Stat->ProgramIdentifier);
		GetTSVToken(File, Stat->TextureIdentifier);
		GetTSVToken(File, Stat->MeshIdentifier);
		GetTSVToken(File, Stat->ColorIdentifier);

		// Check for layer
		std::string LayerIdentifier;
		GetTSVToken(File, LayerIdentifier);
		if(Assets.Layers.find(LayerIdentifier) == Assets.Layers.end())
			throw std::runtime_error("Cannot find layer: " + LayerIdentifier);

		Stat->Layer = Assets.Layers[LayerIdentifier].Layer;

		File >> Stat->Scale;
		File >> Stat->Z;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		return Stat;
	}
	else if(Type == "shape") {
		std::shared_ptr<_ShapeStat> Stat(new _ShapeStat());
		GetTSVToken(File, Stat->Identifier);

		File >> Stat->HalfWidth[0];
		File >> Stat->HalfWidth[1];
		File >> Stat->HalfWidth[2];
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		return Stat;
	}
	else if(Type == "zone") {
		std::shared_ptr<_ZoneStat> Stat(new _ZoneStat());
		GetTSVToken(File, Stat->Identifier);

		return Stat;
	}
	else if(Type == "shot") {
		std::shared_ptr<_ShotStat> Stat(new _ShotStat());
		GetTSVToken(File, Stat->Identifier);

		return Stat;
	}
	else if(Type == "health") {
		std::shared_ptr<_HealthStat> Stat(new _HealthStat());
		GetTSVToken(File, Stat->Identifier);

		File >> Stat->Health;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		return Stat;
	}
	else if(Type == "ai") {
		std::shared_ptr<_AiStat> Stat(new _AiStat());
		GetTSVToken(File, Stat->Identifier);

		return Stat;
	}

	return nullptr;
}
