// CSDK_ChromaGameLoopSample.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Razer\ChromaAnimationAPI.h"
#include <array>
#include <chrono>
#include <conio.h>
#include <iostream>
#include <string>
#include <time.h>
#include <thread>

using namespace ChromaSDK;
using namespace std;

const float MATH_PI = 3.14159f;

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

//ref: https://stackoverflow.com/questions/10905892/equivalent-of-gettimeday-for-windows
int gettimeofday(struct timeval* tp, struct timezone* tzp) {
	namespace sc = std::chrono;
	sc::system_clock::duration d = sc::system_clock::now().time_since_epoch();
	sc::seconds s = sc::duration_cast<sc::seconds>(d);
	tp->tv_sec = (long)s.count();
	tp->tv_usec = (long)sc::duration_cast<sc::microseconds>(d - s).count();

	return 0;
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
		// get time
		struct timeval tp;
		gettimeofday(&tp, NULL);
		long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

		// start with a blank frame
		memset(colors, 0, sizeof(int) * size);

		// add rainbow colors
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

		// add or blend spiral
		if (_sFrameSpiral >= 0)
		{
			if (_sFrameSpiral < ChromaAnimationAPI::GetFrameCountName(ANIMATION_SPIRAL))
			{
				ChromaAnimationAPI::SetCurrentFrame(animationSpiral, _sFrameSpiral);
				int frameCount = ChromaAnimationAPI::GetFrameCountName(ANIMATION_SPIRAL);
				cout << "Spiral: " << (1 + ChromaAnimationAPI::GetCurrentFrameName(ANIMATION_SPIRAL)) << " of " << frameCount << endl;;
				float duration;
				ChromaAnimationAPI::GetFrame(animationSpiral, _sFrameSpiral, &duration, tempColors, size);
				for (int i = 0; i < size; ++i)
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

		// Show hotkeys
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_ESC, 255, 255, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_W, 255, 0, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_A, 255, 0, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_S, 255, 0, 0);
		SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_D, 255, 0, 0);

		// Highlight R if rainbow is active
		if (_sFrameRainbow >= 0)
		{
			SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_R, 0, 255, 0);
		}

		// Highlight S if spiral is active
		if (_sFrameSpiral >= 0)
		{
			SetKeyColorRGB(colors, (int)Keyboard::RZKEY::RZKEY_S, 0, 255, 0);
		}

		// SHow health animation
		{
			int keys[] = {
				Keyboard::RZKEY::RZKEY_F1,
				Keyboard::RZKEY::RZKEY_F2,
				Keyboard::RZKEY::RZKEY_F3,
				Keyboard::RZKEY::RZKEY_F4,
				Keyboard::RZKEY::RZKEY_F5,
				Keyboard::RZKEY::RZKEY_F6,
			};
			int keysLength = sizeof(keys) / sizeof(int);

			float t = ms * 0.002f;
			float hp = fabsf(cos(MATH_PI / 2.0f + t));
			for (int i = 0; i < keysLength; ++i) {
				float ratio = (i + 1) / (float)keysLength;
				int color = ChromaAnimationAPI::GetRGB(0, (int)(255 * (1 - hp)), 0);
				if (((i + 1) / ((float)keysLength + 1)) < hp) {
					color = ChromaAnimationAPI::GetRGB(0, 255, 0);
				}
				else {
					color = ChromaAnimationAPI::GetRGB(0, 100, 0);
				}
				int key = keys[i];
				SetKeyColor(colors, key, color);
			}
		}

		// Show ammo animation
		{
			int keys[] = {
				Keyboard::RZKEY::RZKEY_F7,
				Keyboard::RZKEY::RZKEY_F8,
				Keyboard::RZKEY::RZKEY_F9,
				Keyboard::RZKEY::RZKEY_F10,
				Keyboard::RZKEY::RZKEY_F11,
				Keyboard::RZKEY::RZKEY_F12,
			};
			int keysLength = sizeof(keys) / sizeof(int);

			float t = ms * 0.001f;
			float hp = fabsf(cos(MATH_PI / 2.0f + t));
			for (int i = 0; i < keysLength; ++i) {
				float ratio = (i + 1) / (float)keysLength;
				int color = ChromaAnimationAPI::GetRGB((int)(255 * (1 - hp)), (int)(255 * (1 - hp)), 0);
				if (((i + 1) / ((float)keysLength + 1)) < hp) {
					color = ChromaAnimationAPI::GetRGB(255, 255, 0);
				}
				else {
					color = ChromaAnimationAPI::GetRGB(100, 100, 0);
				}
				int key = keys[i];
				SetKeyColor(colors, key, color);
			}
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
