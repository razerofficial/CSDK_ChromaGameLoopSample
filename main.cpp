// CSDK_ChromaGameLoopSample.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Razer\ChromaAnimationAPI.h"
#include <conio.h>
#include <iostream>
#include <string>
#include <thread>

using namespace ChromaSDK;
using namespace std;

// This final animation will have a single frame
// This animation will have looping on
// This animation will be in immediate mode to avoid any caching
// Any color changes will immediately display in the next frame update.
const char* ANIMATION_FINAL = "Dynamic\\Final_Keyboard.chroma";
const char* ANIMATION_RAINBOW = "Animations\\Rainbow_Keyboard.chroma";
const char* ANIMATION_SPIRAL = "Animations\\Spiral_Keyboard.chroma";

static bool _sWaitForExit = true;
static int _sFrameRainbow = -1;
static int _sFrameSpiral = -1;

// Function prototypes
void Cleanup();
void GameLoop();
int GetKeyColorIndex(int row, int column);
void HandleInput();
void Init();
int main();
void SetKeyColor(int* colors, int rzkey, int color);
void SetKeyColorRGB(int* colors, int rzkey, int red, int green, int blue);
void SetupAnimations();

void Init()
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
}

void SetupAnimations()
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
	SetKeyColor(colors, rzkey, ChromaAnimationAPI::GetRGB(red, green, blue));
}

void GameLoop()
{
	int maxRow = ChromaAnimationAPI::GetMaxRow((int)EChromaSDKDevice2DEnum::DE_Keyboard);
	int maxColumns = ChromaAnimationAPI::GetMaxColumn((int)EChromaSDKDevice2DEnum::DE_Keyboard);
	int size = maxRow * maxColumns;
	int* colors = new int[size];
	int* tempColors = new int[size];
	int animationId = ChromaAnimationAPI::GetAnimation(ANIMATION_FINAL);

	int animationSpiral = ChromaAnimationAPI::GetAnimation(ANIMATION_SPIRAL);
	int animationRainbow = ChromaAnimationAPI::GetAnimation(ANIMATION_RAINBOW);

	while (_sWaitForExit)
	{
		memset(colors, 0, sizeof(int) * size);

		if (_sFrameRainbow >= 0)
		{
			if (_sFrameRainbow < ChromaAnimationAPI::GetFrameCountName(ANIMATION_RAINBOW))
			{
				ChromaAnimationAPI::SetCurrentFrame(animationRainbow, _sFrameRainbow);
				cout << "Rainbow: " << (1 + ChromaAnimationAPI::GetCurrentFrameName(ANIMATION_RAINBOW)) << " of " << ChromaAnimationAPI::GetFrameCountName(ANIMATION_RAINBOW) << endl;;
				float duration;
				ChromaAnimationAPI::GetFrame(animationRainbow, _sFrameRainbow, &duration, tempColors, size);
				memcpy(colors, tempColors, sizeof(int) * size);
				++_sFrameRainbow;
			}
			else
			{
				_sFrameRainbow = -1;
			}
		}

		if (_sFrameSpiral >= 0)
		{
			if (_sFrameSpiral < ChromaAnimationAPI::GetFrameCountName(ANIMATION_SPIRAL))
			{
				ChromaAnimationAPI::SetCurrentFrame(animationSpiral, _sFrameSpiral);
				int frameCount = ChromaAnimationAPI::GetFrameCountName(ANIMATION_SPIRAL);
				cout << "Spiral: " << (1 + ChromaAnimationAPI::GetCurrentFrameName(ANIMATION_SPIRAL)) << " of " << frameCount << endl;;
				float duration;
				ChromaAnimationAPI::GetFrame(animationSpiral, _sFrameSpiral, &duration, tempColors, size);
				for (int i = 0; i < frameCount; ++i)
				{
					if (tempColors[i] != 0)
					{
						colors[i] = tempColors[i];
					}
				}
				++_sFrameSpiral;
			}
			else
			{
				_sFrameSpiral = -1;
			}
		}

		// Set hotkeys
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_ESC, 255, 255, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_W, 255, 0, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_A, 255, 0, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_S, 255, 0, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_D, 255, 0, 0);

		if (_sFrameRainbow >= 0)
		{
			SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_R, 0, 255, 0);
		}

		if (_sFrameSpiral >= 0)
		{
			SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_S, 0, 255, 0);
		}

		ChromaAnimationAPI::UpdateFrame(animationId, 0, 0.033f, colors, size);

		// display the change
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL, 0);

		Sleep(33); //30 FPS
	}
	delete[] colors;
	delete[] tempColors;
}

void HandleInput()
{
	while (_sWaitForExit)
	{
		int input = _getch();
		switch (input)
		{
		case 27:
			_sWaitForExit = false;
			break;
		case 'r':
		case 'R':
			_sFrameRainbow = 0; //start
			break;
		case 's':
		case 'S':
			_sFrameSpiral = 0; //start
			break;
		}
		Sleep(0);
	}
}

void Cleanup()
{
	ChromaAnimationAPI::StopAll();
	ChromaAnimationAPI::CloseAll();
	RZRESULT result = ChromaAnimationAPI::Uninit();
	if (result != RZRESULT_SUCCESS)
	{
		cerr << "Failed to uninitialize Chroma!" << endl;
		exit(1);
	}
}

int main()
{
	Init();
	SetupAnimations();
	thread thread(GameLoop);
	cout << "Press `ESC` to Quit." << endl;
	cout << "Press `R` for rainbow." << endl;
	cout << "Press `S` for spiral." << endl;
	HandleInput();
	thread.join();
	Cleanup();
	return 0;
}
