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
#include <stats.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/controller.h>
#include <objects/animation.h>
#include <objects/render.h>
#include <objects/shape.h>
#include <objects/zone.h>
#include <objects/shot.h>
#include <assets.h>
#include <utils.h>
#include <constants.h>
#include <fstream>
#include <stdexcept>
#include <iostream>

static std::vector<std::string> Components = {
	"physics",
	"controller",
	"animation",
	"render",
	"shape",
	"zone",
	"shot",
};

// Constructor
_Stats::_Stats() {
	LoadPhysics(STATS_PHYSICS);
	LoadControllers(STATS_CONTROLLERS);
	LoadAnimations(STATS_ANIMATIONS);
	LoadRenders(STATS_RENDERS);
	LoadShapes(STATS_SHAPES);
	LoadZones(STATS_ZONES);
	LoadShots(STATS_SHOTS);
	LoadObjects(STATS_OBJECTS);
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
			Object->Physics->Interpolate = false;
	}

	// Create controller
	{
		const auto &ComponentIterator = ObjectStat.Components.find("controller");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Controller = new _Controller(Object, (const _ControllerStat *)ComponentIterator->second.get());
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
		}
	}

	// Create shape
	{
		const auto &ComponentIterator = ObjectStat.Components.find("shape");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Shape = new _Shape(Object, (const _ShapeStat *)ComponentIterator->second.get());
		}
	}

	// Create zone
	{
		const auto &ComponentIterator = ObjectStat.Components.find("zone");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Zone = new _Zone(Object, (const _ZoneStat *)ComponentIterator->second.get());
		}
	}

	// Create shot
	{
		const auto &ComponentIterator = ObjectStat.Components.find("shot");
		if(ComponentIterator != ObjectStat.Components.end()) {
			Object->Shot = new _Shot(Object, (const _ShotStat *)ComponentIterator->second.get());
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
	File.ignore(1024, '\n');

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
		File.ignore(1024, '\n');

		// Add row
		Objects[ObjectStat.Identifier] = ObjectStat;
	}

	// Close file
	File.close();
}

// Load physics components
void _Stats::LoadPhysics(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_PhysicsStat> PhysicsStat(new _PhysicsStat());
		GetTSVToken(File, PhysicsStat->Identifier);
		File >> PhysicsStat->CollisionResponse;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(ComponentStats["physics"].find(PhysicsStat->Identifier) != ComponentStats["physics"].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + PhysicsStat->Identifier);

		// Add row
		ComponentStats["physics"][PhysicsStat->Identifier] = PhysicsStat;
	}

	// Close file
	File.close();
}

// Load controller components
void _Stats::LoadControllers(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_ControllerStat> ControllerStat(new _ControllerStat());
		GetTSVToken(File, ControllerStat->Identifier);
		File >> ControllerStat->Speed;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(ComponentStats["controller"].find(ControllerStat->Identifier) != ComponentStats["controller"].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ControllerStat->Identifier);

		// Add row
		ComponentStats["controller"][ControllerStat->Identifier] = ControllerStat;
	}

	// Close file
	File.close();
}

// Load animation components
void _Stats::LoadAnimations(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_AnimationStat> AnimationStat(new _AnimationStat());
		GetTSVToken(File, AnimationStat->Identifier);

		// Read animation templates
		bool EndOfLine = false;
		while(!EndOfLine && File.peek() != EOF) {
			std::string Token;
			GetTSVToken(File, Token, &EndOfLine);

			if(Token != "")
				AnimationStat->Templates.push_back(Token);
		}

		File.ignore(1024, '\n');

		// Check for duplicates
		if(ComponentStats["animation"].find(AnimationStat->Identifier) != ComponentStats["animation"].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + AnimationStat->Identifier);

		// Add row
		ComponentStats["animation"][AnimationStat->Identifier] = AnimationStat;
	}

	// Close file
	File.close();
}

// Load render components
void _Stats::LoadRenders(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_RenderStat> RenderStat(new _RenderStat());
		GetTSVToken(File, RenderStat->Identifier);
		GetTSVToken(File, RenderStat->ProgramIdentifier);
		GetTSVToken(File, RenderStat->TextureIdentifier);
		GetTSVToken(File, RenderStat->MeshIdentifier);
		GetTSVToken(File, RenderStat->ColorIdentifier);

		std::string LayerIdentifier;
		GetTSVToken(File, LayerIdentifier);
		if(Assets.Layers.find(LayerIdentifier) == Assets.Layers.end())
			throw std::runtime_error("Cannot find layer: " + LayerIdentifier);

		RenderStat->Layer = Assets.Layers[LayerIdentifier].Layer;

		File >> RenderStat->Scale;
		File >> RenderStat->Z;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(ComponentStats["render"].find(RenderStat->Identifier) != ComponentStats["render"].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + RenderStat->Identifier);

		// Add row
		ComponentStats["render"][RenderStat->Identifier] = RenderStat;
	}

	// Close file
	File.close();
}

// Load shape components
void _Stats::LoadShapes(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_ShapeStat> ShapeStat(new _ShapeStat());
		GetTSVToken(File, ShapeStat->Identifier);
		File >> ShapeStat->HalfWidth[0];
		File >> ShapeStat->HalfWidth[1];
		File >> ShapeStat->HalfWidth[2];

		File.ignore(1024, '\n');

		// Check for duplicates
		if(ComponentStats["shape"].find(ShapeStat->Identifier) != ComponentStats["shape"].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ShapeStat->Identifier);

		// Add row
		ComponentStats["shape"][ShapeStat->Identifier] = ShapeStat;
	}

	// Close file
	File.close();
}

// Load zones
void _Stats::LoadZones(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_ZoneStat> ZoneStat(new _ZoneStat());
		GetTSVToken(File, ZoneStat->Identifier);

		//File.ignore(1024, '\n');

		// Check for duplicates
		if(ComponentStats["zone"].find(ZoneStat->Identifier) != ComponentStats["zone"].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ZoneStat->Identifier);

		// Add row
		ComponentStats["zone"][ZoneStat->Identifier] = ZoneStat;
	}

	// Close file
	File.close();
}

// Load shots
void _Stats::LoadShots(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		std::shared_ptr<_ShotStat> ShotStat(new _ShotStat());
		GetTSVToken(File, ShotStat->Identifier);

		//File.ignore(1024, '\n');

		// Check for duplicates
		if(ComponentStats["shot"].find(ShotStat->Identifier) != ComponentStats["shot"].end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ShotStat->Identifier);

		// Add row
		ComponentStats["shot"][ShotStat->Identifier] = ShotStat;
	}

	// Close file
	File.close();
}
