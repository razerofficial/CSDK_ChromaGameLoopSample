// CSDK_ChromaGameLoopSample.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Razer\ChromaAnimationAPI.h"
#include <iostream>
#include <string>

using namespace ChromaSDK;
using namespace std;

// This final animation will have a single frame
// This animation will have looping on
// This animation will be in immediate mode to avoid any caching
// Any color changes will immediately display in the next frame update.
const char* ANIMATION_FINAL = "Dynamic\\Final_Keyboard.chroma";

void Setup()
{
	// Create a blank animation
	int animationId = ChromaAnimationAPI::CreateAnimationInMemory((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)EChromaSDKDevice2DEnum::DE_Keyboard);
	ChromaAnimationAPI::OverrideFrameDuration(animationId, 0.033f);
	ChromaAnimationAPI::CopyAnimation(animationId, ANIMATION_FINAL);
	ChromaAnimationAPI::CloseAnimation(animationId);

	// Show changes after making color changes without loading/caching effects
	ChromaAnimationAPI::UsePreloadingName(ANIMATION_FINAL, false);

	// Clear the cache
	ChromaAnimationAPI::UnloadAnimationName(ANIMATION_FINAL);
}

void CleanUp()
{
	ChromaAnimationAPI::StopAll();
	ChromaAnimationAPI::CloseAll();
}

int GetKeyColorIndex(int row, int column)
{
	return Keyboard::MAX_COLUMN * row + column;
}

void SetKeyColor(int* colors, int rzkey, int color)
{
	int row = HIBYTE(rzkey);
	int column = LOBYTE(rzkey);
	colors[GetKeyColorIndex(row, column)] = color;
}

void SetKeyColorRGB(int* colors, int rzkey, int red, int green, int blue)
{
	SetKeyColor(colors, rzkey, ChromaAnimationAPI::GetRGB(255, 255, 0));
}

void GameLoop()
{
	int maxRow = ChromaAnimationAPI::GetMaxRow((int)EChromaSDKDevice2DEnum::DE_Keyboard);
	int maxColumns = ChromaAnimationAPI::GetMaxColumn((int)EChromaSDKDevice2DEnum::DE_Keyboard);
	int size = maxRow * maxColumns;
	int* colors = new int[size];
	int animationId = ChromaAnimationAPI::GetAnimation(ANIMATION_FINAL);
	while (true)
	{
		memset(colors, 0, sizeof(int) * size);
		SetKeyColor(colors, (int)Keyboard::RZKEY::RZKEY_ESC, ChromaAnimationAPI::GetRGB(255, 255, 0));

		ChromaAnimationAPI::UpdateFrame(animationId, 0, 0.033f, colors, size);

		// display the change
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL, 0);

		Sleep(33); //30 FPS
	}
	delete[] colors;
}

int main()
{
	if (ChromaAnimationAPI::InitAPI() != 0)
	{
		cerr << "Failed to load Chroma library!" << endl;
		exit(1);
	}
	RZRESULT result = ChromaAnimationAPI::Init();
	if (result != RZRESULT_SUCCESS)
	{
		cerr << "Failed to initialize Chroma!" << endl;
		exit(1);
	}
	Sleep(100); //wait for init
	Setup();
	GameLoop();
	result = ChromaAnimationAPI::Uninit();
	if (result != RZRESULT_SUCCESS)
	{
		cerr << "Failed to uninitialize Chroma!" << endl;
		exit(1);
	}
}
