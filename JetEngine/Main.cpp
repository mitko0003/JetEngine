#include "Precompiled.h"

#include <stdlib.h>

#include "WindowContext-nt.h"
#include "FileSystem.h"
#include "RenderDevice-vk.h"
#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"
#include "Core/Containers/String.h"
//#include "tiny_obj_loader.h"

//#define CONSOLE

#if defined(CONSOLE)

int main()
{

}

#else

struct TVertex
{
	Vector3 Position;
};

struct TMesh
{
	TVarArray<TVertex> TVertexBuffer;
	TVarArray<uint16> IndexBuffer;
};

void MainLoop(TVulkanAPI *graphicsAPI)
{
	//TVertex vertexBuffer[] = {
	//	{ { -1.0f, -1.0f, 0.0f } },
	//	{ { -1.0f,  1.0f, 0.0f } },
	//	{ {  1.0f,  1.0f, 0.0f } }
	//};
	//int32 indexBuffer[] = {
	//	0, 1, 2
	//};
	//
	//constexpr int32 width = 1024, height = 1024;
	//
	//float depthBuffer[width][height] = {};
	//
	//Vector2 pixelSize(1.0f / width, 1.0f / height);
	//for (int32 i = 0; i < ArrayLength(indexBuffer); i += 3)
	//{
	//	const auto &v0 = vertexBuffer[indexBuffer[i + 0]].Position;
	//	const auto &v1 = vertexBuffer[indexBuffer[i + 1]].Position;
	//	const auto &v2 = vertexBuffer[indexBuffer[i + 2]].Position;
	//
	//	const auto [left, right] = MinMax(v0.x, v1.x, v2.x);
	//	const auto [bottom, top] = MinMax(v0.y, v1.y, v2.y);
	//}

	auto objFile = FS::Open("Test/teapot.obj", FS::Read);

	TVarArray<Vector3> Positions;
	TVarArray<Vector3> Normals;
	TVarArray<Vector2> TextureCoords;
	TVarArray<Vector3i> Faces;

	const auto *line = reinterpret_cast<const char*>(objFile.Platform.Buffer);

	const auto NextLine = [&]() {
		while (*line++ != '\n');
	};

	for (; *line != '\0'; NextLine())
	{
		if (*line == '#' || *line == '\n' || *line == '\r')
			continue;
		if (StringCompareN(line, STR_AND_LEN("mtllib")) == 0)
			continue;
		if (StringCompareN(line, STR_AND_LEN("g ")) == 0)
			continue;
		if (StringCompareN(line, STR_AND_LEN("v ")) == 0)
		{
			auto &position = Positions.push_back({});
			sscanf_s(line, "v %f %f %f", &position.x, &position.y, &position.z);
			continue;
		}
		if (StringCompareN(line, STR_AND_LEN("vn ")) == 0)
		{
			auto &normal = Normals.push_back({});
			sscanf_s(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z);
			continue;
		}
		if (StringCompareN(line, STR_AND_LEN("vt ")) == 0)
		{
			auto &textureCoord = TextureCoords.push_back({});
			sscanf_s(line, "vt %f %f", &textureCoord.x, &textureCoord.y);
			continue;
		}
		if (StringCompareN(line, STR_AND_LEN("f ")) == 0)
		{
			line += 2;
			auto &face = Faces.push_back({});
			while (*line != '\r' && *line != '\n')
			{
				int32 read = 0;
				sscanf_s(line, "%d/%d/%d%n", &face.x, &face.y, &face.z, &read);

				line += read;
				while (*line == ' ')
					++line;
			}
			continue;
		}
	}
		

	graphicsAPI->HelloWorld();
}

INT WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, INT nCmdShow)
{
    // Register the window class.
	char cwd[2048];
	GetCurrentDirectory(sizeof(cwd), cwd);
	DebugPrint<logVerbose>("CWD: %s\n", cwd);

	//tinyobj::attrib_t attrib;
	//std::vector<tinyobj::shape_t> shapes;
	//std::vector<tinyobj::material_t> materials;

	//std::string warn;
	//std::string err;
	//(attrib_t *attrib, std::vector<shape_t> *shapes,
	//	std::vector<material_t> *materials, std::string *warn,
	//	std::string *err, std::istream *inStream,
	//	MaterialReader *readMatFn /*= NULL*/, bool triangulate,
	//	bool default_vcols_fallback)
	//bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err);
	

	TWindowNT window(instance, nCmdShow);
    if (!window.IsValid())
        return 0;

	TVulkanAPI vulkan;
	vulkan.Init(&window);

    // Run the message loop.
    MSG message = {};
    while (true)
    {
		//PeekMessage(&message, NULL, 0, 0, PM_REMOVE);
		//switch (message.message)
		//{
		//
		//}
        MainLoop(&vulkan);
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

	vulkan.Done();
    return 0;
}

#endif