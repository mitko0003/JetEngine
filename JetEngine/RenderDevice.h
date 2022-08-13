#pragma once

#include "WindowContext.h"

class IGraphicsAPI;

class ISwapChain
{

};

class IGraphicsTexture
{

};

class IGraphicsBuffer
{
public:
	IGraphicsBuffer(IGraphicsAPI *parentDevice) :
		ParentDevice(parentDevice) {}

protected:
	IGraphicsAPI *ParentDevice;
};

class IGraphicsAPI
{
public:
	virtual void Init(const IWindow *) = 0;
	virtual void Done() = 0;

	virtual IGraphicsBuffer *CreateBuffer(int32 size) = 0;
}; // class IGraphicsAPI