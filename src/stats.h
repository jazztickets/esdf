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
#include <glm/vec4.hpp>
#include <unordered_map>
#include <string>
#include <vector>

// Forward Declarations
class _Object;
class _Prop;
struct _ItemStat;

// Physics template
struct _PhysicsStat {
	std::string Identifier;
	float Radius;
};

// Controller template
struct _ControllersStat {
	std::string Identifier;
	float Speed;
};

// Animation template
struct _AnimationsStat {
	std::string Identifier;
	std::vector<std::string> Templates;
};

// Render template
struct _RendersStat {
	std::string Identifier;
	std::string Icon;
	float Scale;
	float Z;
	int Layer;
};

// Shape template
struct _ShapeStat {
	std::string Identifier;
	glm::vec4 AABB;
};

// Objects template
struct _ObjectStat {
	std::string Identifier;
	std::string Name;
	const _PhysicsStat *PhysicsStat;
	const _ControllersStat *ControllersStat;
	const _AnimationsStat *AnimationsStat;
	const _RendersStat *RendersStat;
	const _ShapeStat *ShapeStat;
};

// Prop template
struct _PropStat {
	std::string Identifier;
	std::string MeshIdentifier;
	std::string TextureIdentifier;
	float Radius;
};

// Classes
class _Stats {

	public:

		_Stats();
		~_Stats();

		_Object *CreateObject(const std::string Identifier, bool IsServer) const;
		_Prop *CreateProp(const std::string Identifier) const;

		std::unordered_map<std::string, _ObjectStat> Objects;
		std::unordered_map<std::string, _PropStat> Props;

	private:

		void LoadObjects(const std::string &Path);
		void LoadPhysics(const std::string &Path);
		void LoadControllers(const std::string &Path);
		void LoadAnimations(const std::string &Path);
		void LoadRenders(const std::string &Path);
		void LoadShapes(const std::string &Path);
		void LoadProps(const std::string &Path);

		std::unordered_map<std::string, _PhysicsStat> Physics;
		std::unordered_map<std::string, _ControllersStat> Controllers;
		std::unordered_map<std::string, _AnimationsStat> Animations;
		std::unordered_map<std::string, _RendersStat> Renders;
		std::unordered_map<std::string, _ShapeStat> Shapes;

};
