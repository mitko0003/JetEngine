#pragma once

namespace FS
{
	struct File
	{
		uint64 uSize = 0;
		LPVOID pBuf = nullptr;
		HANDLE hMapFile = INVALID_HANDLE_VALUE;
		HANDLE hFile = INVALID_HANDLE_VALUE;
	};

	File Open(const char *path);
	void Close(File file);
}