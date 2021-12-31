#pragma once

class ISwapChain
{

};

class IGraphicsAPI
{
public:
	virtual void Init(HINSTANCE instance, HWND hwnd) = 0;
	virtual void Done() = 0;
}; // class IGraphicsAPI