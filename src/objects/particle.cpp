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
#include <objects/particle.h>
#include <particles.h>
#include <graphics.h>

// Constructor
_Particle::_Particle(const _ParticleSpawn &Spawn)
:	Type(Spawn.Template->Type),
	Lifetime(Spawn.Template->Lifetime),
	Deleted(false),
	Texture(Spawn.Template->Texture),
	Color(Spawn.Template->Color),
	AlphaSpeed(Spawn.Template->AlphaSpeed),
	PositionZ(Spawn.PositionZ),
	ScaleAspect(Spawn.Template->ScaleAspect),
	Radius(0.0f) {

}

// Destructor
_Particle::~_Particle() {
}

// Update
void _Particle::Update(double FrameTime) {
	Position += Velocity;
	Velocity += Acceleration;
	Rotation += TurnSpeed;
	Color.a += AlphaSpeed;
	Lifetime -= FrameTime;

	if(Color.a < 0.0f)
		Color.a = 0.0f;

	if(Lifetime < 0)
		Deleted = true;
}

// Render
void _Particle::Render() {

	if(Texture) {
		Graphics.SetColor(Color);
		Graphics.DrawSprite(glm::vec3(Position, PositionZ), Texture, Rotation, Scale);
	}
}
