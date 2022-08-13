#pragma once

class IWindow
{
public:
	using Ptr = TSharedPtr<IWindow>;

	virtual TPair<int32, int32> GetDimensions() const = 0;
	virtual void SetTitle(const char*) const = 0;
};