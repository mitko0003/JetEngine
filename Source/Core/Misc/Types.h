#pragma once

#include <cstdint>

// Note this is not <limits>, which contains std::numeric_limits! Instead it is a C header containing platform defines.
#include <limits.h>

// Initializer lists are a compiler feature, in spite of them being in the std namespace.
// It is possible to provide a "custom" implementation, but it will be compiler specific and this beats the purpose of providing it.
#include <initializer_list>

using uint8   = uint8_t;
using uint16  = uint16_t;
using uint32  = uint32_t;
using uint64  = uint64_t;

using int8    = int8_t;
using int16   = int16_t;
using int32   = int32_t;
using int64   = int64_t;

using float32 = float;
using float64 = double;

using char8   = char;
using char16  = char16_t;
using char32  = char32_t;