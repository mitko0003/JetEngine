#pragma once

#include "Utils.h"

void* operator new(std::size_t) {
	ASSERT(!"Not implemented!");
	return nullptr;
}

void* operator new[](std::size_t) {
	ASSERT(!"Not implemented!");
	return nullptr;
}

void operator delete(void*) {
	ASSERT(!"Not implemented!");
}

void operator delete[](void*) {
	ASSERT(!"Not implemented!");
}