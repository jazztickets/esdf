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
#include <framework.h>
#include <config.h>

int main(int ArgumentCount, char **Arguments) {

	// Init config system
	Config.Init("settings.cfg");

	// Init framework
	Framework.Init(ArgumentCount, Arguments);

	// Run framework
	while(!Framework.GetDone()) {
		Framework.Update();
	}

	// Shutdown
	Framework.Close();
	Config.Close();

	return 0;
}
