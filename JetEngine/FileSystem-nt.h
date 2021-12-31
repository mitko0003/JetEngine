#pragma once

namespace FS
{
	struct FileNT
	{
		LPVOID Buffer     = nullptr;
		HANDLE Mapping    = INVALID_HANDLE_VALUE;
		HANDLE Descriptor = INVALID_HANDLE_VALUE;
	};

	using PlatformFile = FileNT;
}
