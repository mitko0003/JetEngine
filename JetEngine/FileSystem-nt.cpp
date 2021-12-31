#include "Precompiled.h"

#include "FileSystem.h"

namespace FS
{
	File Open(const char *path, AccessMode accessMode)
	{
		File file;
		FileNT &fileNT = file.Platform;
		
		{
			DWORD dwDesiredAccess = 0;
			if (accessMode & AccessMode::Read)    dwDesiredAccess |= GENERIC_READ;
			if (accessMode & AccessMode::Write)   dwDesiredAccess |= GENERIC_WRITE;
			if (accessMode & AccessMode::Execute) dwDesiredAccess |= GENERIC_EXECUTE;

			fileNT.Descriptor = CreateFile(path, dwDesiredAccess, FILE_ATTRIBUTE_READONLY, NULL, OPEN_EXISTING, NULL, NULL);
		}

		{
			DWORD flProtect = PAGE_NOACCESS;
			if (accessMode & AccessMode::Read)    flProtect = PAGE_READONLY;
			if (accessMode & AccessMode::Write)   flProtect = PAGE_READWRITE;
			if (accessMode & AccessMode::Execute) flProtect = PAGE_EXECUTE;
			if (accessMode & AccessMode::Execute && accessMode & AccessMode::Read)  flProtect = PAGE_EXECUTE_READ;
			if (accessMode & AccessMode::Execute && accessMode & AccessMode::Write) flProtect = PAGE_EXECUTE_READWRITE;

			fileNT.Mapping = CreateFileMapping(fileNT.Descriptor, 0, flProtect, 0, 0, nullptr);
			ASSERT(fileNT.Mapping != NULL && fileNT.Mapping != INVALID_HANDLE_VALUE);
		}

		GetFileSizeEx(fileNT.Descriptor, reinterpret_cast<PLARGE_INTEGER>(&file.Size));

		{
			DWORD dwDesiredAccess = 0;
			if (accessMode & AccessMode::Read)    dwDesiredAccess |= FILE_MAP_READ;
			if (accessMode & AccessMode::Write)   dwDesiredAccess |= FILE_MAP_WRITE;
			if (accessMode & AccessMode::Execute) dwDesiredAccess |= FILE_MAP_EXECUTE;

			fileNT.Buffer = MapViewOfFile(fileNT.Mapping, dwDesiredAccess, 0, 0, 0);
		}

		ASSERT(fileNT.Buffer != nullptr);
		return file;
	}

	void Close(File file)
	{
		const FileNT &fileNT = file.Platform;
		UnmapViewOfFile(fileNT.Buffer);
		CloseHandle(fileNT.Mapping);
		CloseHandle(fileNT.Descriptor);
	}
}