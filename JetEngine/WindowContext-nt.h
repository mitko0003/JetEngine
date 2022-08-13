#pragma once

#include "WindowContext.h"

class TWindowNT final : public IWindow
{
public:
    TWindowNT(HINSTANCE, INT nCmdShow);
    ~TWindowNT();

    TPair<int32, int32> GetDimensions() const override;
    void SetTitle(const char *) const override;

    bool IsValid() const {
        return Handle != NULL;
    }

    ATOM Class;
    HWND Handle;
    HINSTANCE Instance;
};