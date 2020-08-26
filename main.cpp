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
	ChromaAnimationAPI::CopyAnimation(animationId, ANIMATION_FINAL);
	ChromaAnimationAPI::CloseAnimation(animationId);

	// Make sure it works
	ChromaAnimationAPI::SetKeyColorName(ANIMATION_FINAL, 0, Keyboard::RZKEY::RZKEY_ESC, ChromaAnimationAPI::GetRGB(255, 255, 0));
}

void CleanUp()
{
	ChromaAnimationAPI::StopAll();
	ChromaAnimationAPI::CloseAll();
}

void GameLoop()
{
	ChromaAnimationAPI::PlayAnimationName(ANIMATION_FINAL, true);
	while (true)
	{
		Sleep(0);
	}
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
	Setup();
	GameLoop();
	result = ChromaAnimationAPI::Uninit();
	if (result != RZRESULT_SUCCESS)
	{
		cerr << "Failed to uninitialize Chroma!" << endl;
		exit(1);
	}
}
