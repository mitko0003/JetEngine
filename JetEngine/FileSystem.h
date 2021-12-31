#pragma once

#include "FileSystem-nt.h"

namespace FS
{
	enum AccessMode : uint32
	{
		None        = 0u,
		Read        = 1u << 0u,
		Write       = 1u << 1u,
		Execute     = 1u << 2u,
		ReadWrite   = Read | Write,
	};

	struct File
	{
		uint64 Size = 0;
		PlatformFile Platform;
	};

	File Open(const char *path, AccessMode accessMode = AccessMode::None);
	void Close(File file);
}