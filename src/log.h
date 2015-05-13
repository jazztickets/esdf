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
#include <fstream>
#include <iostream>

// Log file class
class _Log {

	public:

		_Log() : ToStdOut(false) { }
		~_Log() {
			File.close();
			File.clear();
		}

		// Open log file
		void Open(const char *Filename) {
			File.open(Filename);
		}

		void SetToStdOut(bool ToStdOut) { this->ToStdOut = ToStdOut; }

		// Handles most types
		template <typename Type>
		_Log &operator<<(const Type &Value) {
			if(ToStdOut)
				std::clog << Value;

			if(File.is_open())
				File << Value;

			return *this;
		}

		// Handles endl
		_Log &operator<<(std::ostream &(*Value)(std::ostream &)) {
			if(ToStdOut)
				std::clog << Value;

			if(File.is_open())
				File << Value;

			return *this;
		}

	private:

		std::ofstream File;
		bool ToStdOut;
};
