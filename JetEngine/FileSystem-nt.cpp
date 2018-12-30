#include "Precompiled.h"

#include "FileSystem.h"

namespace FS
{
	File Open(const char *path)
	{
		File file;
		file.hFile = CreateFile(path, GENERIC_READ, FILE_ATTRIBUTE_READONLY, NULL, OPEN_EXISTING, NULL, NULL);

		file.hMapFile = CreateFileMapping(file.hFile, 0, PAGE_READONLY, 0, 0, nullptr);

		ASSERT(file.hMapFile != NULL && file.hMapFile != INVALID_HANDLE_VALUE);

		GetFileSizeEx(file.hFile, reinterpret_cast<PLARGE_INTEGER>(&file.uSize));
		file.pBuf = MapViewOfFile(file.hMapFile, FILE_MAP_READ, 0, 0, 0);

		ASSERT(file.pBuf != nullptr);
		return file;
	}

	void Close(File file)
	{
		UnmapViewOfFile(file.pBuf);
		CloseHandle(file.hMapFile);
		CloseHandle(file.hFile);
	}
}