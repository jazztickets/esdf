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

// Classes
template<class T> class _CircularBuffer {

	public:

		// Constructor
		_CircularBuffer() : Data(nullptr) { }

		// Constructor with size
		_CircularBuffer(int Size) {
			Init(Size);
		}

		// Destructor
		~_CircularBuffer() {
			delete[] Data;
		}

		// Initialize with size
		void Init(int Size) {
			if(Data)
				delete[] Data;

			Data = new T[Size];
			CurrentSize = 0;
			MaxSize = Size;
			ReadIndex = 0;
			WriteIndex = 0;
		}

		// Free memory
		void Close() {
			delete[] Data;
			Data = nullptr;
		}

		// Clear the buffer
		void Clear() {
			CurrentSize = 0;
			ReadIndex = 0;
			WriteIndex = 0;
		}

		// Check if buffer is empty
		bool IsEmpty() const {
			return CurrentSize == 0;
		}

		// Get size
		int Size() const {
			return CurrentSize;
		}

		// Add to back of queue
		void PushBack(const T &Value) {

			// Update size
			CurrentSize++;
			if(CurrentSize > MaxSize) {
				CurrentSize = MaxSize;
				ReadIndex++;
				if(ReadIndex >= MaxSize)
					ReadIndex = 0;
			}

			// Save data
			Data[WriteIndex] = Value;

			// Update pointers
			WriteIndex++;
			if(WriteIndex >= MaxSize)
				WriteIndex = 0;
		}

		// Pop back of queue
		void PopBack() {

			// Check size
			if(CurrentSize == 0)
				return;

			// Update buffer
			CurrentSize--;
			WriteIndex--;
			if(WriteIndex < 0)
				WriteIndex = MaxSize-1;
		}

		// Remove front of queue
		void Pop() {

			// Check size
			if(CurrentSize == 0)
				return;

			// Update buffer
			CurrentSize--;
			ReadIndex++;
			if(ReadIndex >= MaxSize)
				ReadIndex = 0;
		}

		// Get the front of the queue
		T &Front(int Depth = 0) const {
			int Index = ReadIndex + Depth;
			if(Index >= MaxSize)
				Index -= MaxSize;

			return Data[Index];
		}

		// Get the back of the queue
		T &Back(int Depth = 0) const {
			int Index = WriteIndex - Depth - 1;
			if(Index < 0)
				Index += MaxSize;

			return Data[Index];
		}

	private:

		// Data
		T *Data;

		// Size
		int CurrentSize, MaxSize;

		// Pointers
		int ReadIndex, WriteIndex;
};
