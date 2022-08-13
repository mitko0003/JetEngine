#pragma once

#include "TypeTraits.h"

template <typename T>
concept CIntegral = TIsIntegral_v<T>;