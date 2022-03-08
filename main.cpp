// CSDK_ChromaGameLoopSample.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// When true, the sample will set Chroma effects directly from Arrays
// When false, the sample will use dynamic animations that set Chroma effects
// using the first frame of the dynamic animation.
#define USE_ARRAY_EFFECTS true

#include "Razer\ChromaAnimationAPI.h"
#include "HandleInput.h"
#include "CpuUsage.h"
#include <array>
#include <chrono>
#include <conio.h>
#include <iostream>
#include <string>
#include <tchar.h>
#include <time.h>
#include <thread>

using namespace ChromaSDK;
using namespace std;

const float MATH_PI = 3.14159f;

#if !USE_ARRAY_EFFECTS

// This final animation will have a single frame
// Any color changes will immediately display in the next frame update.
const char* ANIMATION_FINAL_CHROMA_LINK = "Dynamic\\Final_ChromaLink.chroma";
const char* ANIMATION_FINAL_HEADSET = "Dynamic\\Final_Headset.chroma";
const char* ANIMATION_FINAL_KEYBOARD = "Dynamic\\Final_Keyboard.chroma";
const char* ANIMATION_FINAL_KEYPAD = "Dynamic\\Final_Keypad.chroma";
const char* ANIMATION_FINAL_MOUSE = "Dynamic\\Final_Mouse.chroma";
const char* ANIMATION_FINAL_MOUSEPAD = "Dynamic\\Final_Mousepad.chroma";

#endif

static bool _sWaitForExit = true;
static bool _sHotkeys = true;
static bool _sAmmo = true;
static int _sAmbientColor = 0;
static int _sIndexLandscape = -1;
static int _sIndexFire = -1;
static int _sIndexRainbow = -1;
static int _sIndexSpiral = -1;

static FChromaSDKScene _sScene;

static char _sShortcode[7] = { 0 };
static char _sStreamId[48] = { 0 };
static char _sStreamKey[48] = { 0 };
static unsigned char _sLenShortcode = 0;
static unsigned char _sLenStreamId = 0;
static unsigned char _sLenStreamKey = 0;

static int _sSelection = 0;
const int MAX_SELECTION = 7;

// Function prototypes
void Cleanup();
void GameLoop();
int GetKeyColorIndex(int row, int column);
void InputHandler();
void Init();
int main();
void SetKeyColor(int* colors, int rzkey, int color);
void SetKeyColorRGB(int* colors, int rzkey, int red, int green, int blue);

const char* IsSelected(int index)
{
	return (index == _sSelection) ? "*" : " ";
}

CpuUsage _gUsage;

void PrintLegend()
{
	for (int i = 0; i < 25; ++i)
	{
		fprintf(stdout, "\r\n");
	}

	fprintf(stdout, "C++ GAME LOOP SAMPLE APP\r\n");
	fprintf(stdout, "\r\n");

	cout << "Press `ESC` to Quit." << endl;
	cout << "Press `C` to change base color." << endl;
	cout << "Press `A` for ammo/health." << endl;
	cout << "Press `H` to toggle hotkeys." << endl;
	cout << "Press `F` for fire." << endl;
	cout << "Press `L` for landscape." << endl;
	cout << "Press `R` for rainbow." << endl;
	cout << "Press `S` for spiral." << endl;
	
	short cpuUsage = _gUsage.GetUsage();
	cout << "CPU usage: " << cpuUsage << "%" << endl;

	fprintf(stdout, "Use UP and DOWN arrows to select item and press ENTER.\r\n");
	fprintf(stdout, "Use ESCAPE to QUIT.\r\n");
	fprintf(stdout, "\r\n");

	if (ChromaAnimationAPI::CoreStreamSupportsStreaming())
	{
		fprintf(stdout, "Streaming Info (SUPPORTED):\r\n");
		ChromaSDK::Stream::StreamStatusType status = ChromaAnimationAPI::CoreStreamGetStatus();
		fprintf(stdout, "Status: %s\r\n", ChromaAnimationAPI::CoreStreamGetStatusString(status));
		if (_sLenShortcode > 0)
		{
			fprintf(stdout, "Shortcode: %s\r\n", _sShortcode);
		}
		if (_sLenStreamId > 0)
		{
			fprintf(stdout, "StreamId: %s\r\n", _sStreamId);
		}
		if (_sLenStreamKey > 0)
		{
			fprintf(stdout, "StreamKey: %s\r\n", _sStreamKey);
		}
		fprintf(stdout, "\r\n");
	}

	int index = -1;
	if (ChromaAnimationAPI::CoreStreamSupportsStreaming())
	{
		fprintf(stdout, "[%s] Request Shortcode\r\n", IsSelected(++index));
		fprintf(stdout, "[%s] Request StreamId\r\n", IsSelected(++index));
		fprintf(stdout, "[%s] Request StreamKey\r\n", IsSelected(++index));
		fprintf(stdout, "[%s] Release Shortcode\r\n", IsSelected(++index));
		fprintf(stdout, "[%s] Broadcast\r\n", IsSelected(++index));
		fprintf(stdout, "[%s] BroadcastEnd\r\n", IsSelected(++index));
		fprintf(stdout, "[%s] Watch\r\n", IsSelected(++index));
		fprintf(stdout, "[%s] WatchEnd\r\n", IsSelected(++index));
	}	
}

void Init()
{
	if (ChromaAnimationAPI::InitAPI() != 0)
	{
		cerr << "Failed to load Chroma library!" << endl;
		exit(1);
	}

	ChromaSDK::APPINFOTYPE appInfo = {};

	_tcscpy_s(appInfo.Title, 256, _T("Razer Chroma CSDK Game Loop Sample Application"));
	_tcscpy_s(appInfo.Description, 1024, _T("A sample application using Razer Chroma SDK"));
	_tcscpy_s(appInfo.Author.Name, 256, _T("Razer"));
	_tcscpy_s(appInfo.Author.Contact, 256, _T("https://developer.razer.com/chroma"));

	//appInfo.SupportedDevice = 
	//    0x01 | // Keyboards
	//    0x02 | // Mice
	//    0x04 | // Headset
	//    0x08 | // Mousepads
	//    0x10 | // Keypads
	//    0x20   // ChromaLink devices
	appInfo.SupportedDevice = (0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20);
	//    0x01 | // Utility. (To specifiy this is an utility application)
	//    0x02   // Game. (To specifiy this is a game);
	appInfo.Category = 1;

	RZRESULT result = ChromaAnimationAPI::InitSDK(&appInfo);
	if (result != RZRESULT_SUCCESS)
	{
		cerr << "Failed to initialize Chroma!" << endl;
		ChromaAnimationAPI::UnloadLibrarySDK();
		exit(1);
	}
	Sleep(100); //wait for init
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

const int GetColorArraySize1D(EChromaSDKDevice1DEnum device)
{
	const int maxLeds = ChromaAnimationAPI::GetMaxLeds((int)device);
	return maxLeds;
}

const int GetColorArraySize2D(EChromaSDKDevice2DEnum device)
{
	const int maxRow = ChromaAnimationAPI::GetMaxRow((int)device);
	const int maxColumn = ChromaAnimationAPI::GetMaxColumn((int)device);
	return maxRow * maxColumn;
}

#if !USE_ARRAY_EFFECTS

void SetupAnimation1D(const char* path, EChromaSDKDevice1DEnum device)
{
	int animationId = ChromaAnimationAPI::GetAnimation(path);
	if (animationId == -1)
	{
		animationId = ChromaAnimationAPI::CreateAnimationInMemory((int)EChromaSDKDeviceTypeEnum::DE_1D, (int)device);
		ChromaAnimationAPI::CopyAnimation(animationId, path);
		ChromaAnimationAPI::CloseAnimation(animationId);
		ChromaAnimationAPI::MakeBlankFramesName(path, 1, 0.1f, 0);
	}
}

void SetupAnimation2D(const char* path, EChromaSDKDevice2DEnum device)
{
	int animationId = ChromaAnimationAPI::GetAnimation(path);
	if (animationId == -1)
	{
		animationId = ChromaAnimationAPI::CreateAnimationInMemory((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)device);
		ChromaAnimationAPI::CopyAnimation(animationId, path);
		ChromaAnimationAPI::CloseAnimation(animationId);
		ChromaAnimationAPI::MakeBlankFramesName(path, 1, 0.1f, 0);
	}
}
#endif

void SetStaticColor(int* colors, int color, int size)
{
	for (int i = 0; i < size; ++i)
	{
		colors[i] = color;
	}
}

int MultiplyColor(int color1, int color2) {
	int redColor1 = color1 & 0xFF;
	int greenColor1 = (color1 >> 8) & 0xFF;
	int blueColor1 = (color1 >> 16) & 0xFF;

	int redColor2 = color2 & 0xFF;
	int greenColor2 = (color2 >> 8) & 0xFF;
	int blueColor2 = (color2 >> 16) & 0xFF;

	int red = (int)floor(255 * ((redColor1 / 255.0f) * (redColor2 / 255.0f)));
	int green = (int)floor(255 * ((greenColor1 / 255.0f) * (greenColor2 / 255.0f)));
	int blue = (int)floor(255 * ((blueColor1 / 255.0f) * (blueColor2 / 255.0f)));

	return ChromaAnimationAPI::GetRGB(red, green, blue);
}

int AverageColor(int color1, int color2) {
	return ChromaAnimationAPI::LerpColor(color1, color2, 0.5f);
}

int AddColor(int color1, int color2) {
	int redColor1 = color1 & 0xFF;
	int greenColor1 = (color1 >> 8) & 0xFF;
	int blueColor1 = (color1 >> 16) & 0xFF;

	int redColor2 = color2 & 0xFF;
	int greenColor2 = (color2 >> 8) & 0xFF;
	int blueColor2 = (color2 >> 16) & 0xFF;

	int red = min(redColor1 + redColor2, 255) & 0xFF;
	int green = min(greenColor1 + greenColor2, 255) & 0xFF;
	int blue = min(blueColor1 + blueColor2, 255) & 0xFF;

	return ChromaAnimationAPI::GetRGB(red, green, blue);
}

int SubtractColor(int color1, int color2) {
	int redColor1 = color1 & 0xFF;
	int greenColor1 = (color1 >> 8) & 0xFF;
	int blueColor1 = (color1 >> 16) & 0xFF;

	int redColor2 = color2 & 0xFF;
	int greenColor2 = (color2 >> 8) & 0xFF;
	int blueColor2 = (color2 >> 16) & 0xFF;

	int red = max(redColor1 - redColor2, 0) & 0xFF;
	int green = max(greenColor1 - greenColor2, 0) & 0xFF;
	int blue = max(blueColor1 - blueColor2, 0) & 0xFF;

	return ChromaAnimationAPI::GetRGB(red, green, blue);
}

int MaxColor(int color1, int color2) {
	int redColor1 = color1 & 0xFF;
	int greenColor1 = (color1 >> 8) & 0xFF;
	int blueColor1 = (color1 >> 16) & 0xFF;

	int redColor2 = color2 & 0xFF;
	int greenColor2 = (color2 >> 8) & 0xFF;
	int blueColor2 = (color2 >> 16) & 0xFF;

	int red = max(redColor1, redColor2) & 0xFF;
	int green = max(greenColor1, greenColor2) & 0xFF;
	int blue = max(blueColor1, blueColor2) & 0xFF;

	return ChromaAnimationAPI::GetRGB(red, green, blue);
}

int MinColor(int color1, int color2) {
	int redColor1 = color1 & 0xFF;
	int greenColor1 = (color1 >> 8) & 0xFF;
	int blueColor1 = (color1 >> 16) & 0xFF;

	int redColor2 = color2 & 0xFF;
	int greenColor2 = (color2 >> 8) & 0xFF;
	int blueColor2 = (color2 >> 16) & 0xFF;

	int red = min(redColor1, redColor2) & 0xFF;
	int green = min(greenColor1, greenColor2) & 0xFF;
	int blue = min(blueColor1, blueColor2) & 0xFF;

	return ChromaAnimationAPI::GetRGB(red, green, blue);
}

int InvertColor(int color) {
	int red = 255 - (color & 0xFF);
	int green = 255 - ((color >> 8) & 0xFF);
	int blue = 255 - ((color >> 16) & 0xFF);

	return ChromaAnimationAPI::GetRGB(red, green, blue);
}

int MultiplyNonZeroTargetColorLerp(int color1, int color2, int inputColor) {
	if (inputColor == 0)
	{
		return inputColor;
	}
	float red = (inputColor & 0xFF) / 255.0f;
	float green = ((inputColor & 0xFF00) >> 8) / 255.0f;
	float blue = ((inputColor & 0xFF0000) >> 16) / 255.0f;
	float t = (red + green + blue) / 3.0f;
	return ChromaAnimationAPI::LerpColor(color1, color2, t);
}

int Thresh(int color1, int color2, int inputColor) {
	float red = (inputColor & 0xFF) / 255.0f;
	float green = ((inputColor & 0xFF00) >> 8) / 255.0f;
	float blue = ((inputColor & 0xFF0000) >> 16) / 255.0f;
	float t = (red + green + blue) / 3.0f;
	if (t == 0.0)
	{
		return 0;
	}
	if (t < 0.5)
	{
		return color1;
	}
	else
	{
		return color2;
	}
}


void BlendAnimation1D(const FChromaSDKSceneEffect& effect, FChromaSDKDeviceFrameIndex& deviceFrameIndex, int device, EChromaSDKDevice1DEnum device1d, const char* animationName,
	int* colors, int* tempColors)
{
	const int size = GetColorArraySize1D(device1d);
	const int frameId = deviceFrameIndex._mFrameIndex[device];
	const int frameCount = ChromaAnimationAPI::GetFrameCountName(animationName);
	if (frameId < frameCount)
	{
		//cout << animationName << ": " << (1 + frameId) << " of " << frameCount << endl;
		float duration;
		int animationId = ChromaAnimationAPI::GetAnimation(animationName);
		ChromaAnimationAPI::GetFrame(animationId, frameId, &duration, tempColors, size);
		for (int i = 0; i < size; ++i)
		{
			int color1 = colors[i]; //target
			int tempColor = tempColors[i]; //source

			// BLEND
			int color2;
			switch (effect._mBlend)
			{
			case EChromaSDKSceneBlend::SB_None:
				color2 = tempColor; //source
				break;
			case EChromaSDKSceneBlend::SB_Invert:
				if (tempColor != 0) //source
				{
					color2 = InvertColor(tempColor); //source inverted
				}
				else
				{
					color2 = 0;
				}
				break;
			case EChromaSDKSceneBlend::SB_Threshold:
				color2 = Thresh(effect._mPrimaryColor, effect._mSecondaryColor, tempColor); //source
				break;
			case EChromaSDKSceneBlend::SB_Lerp:
			default:
				color2 = MultiplyNonZeroTargetColorLerp(effect._mPrimaryColor, effect._mSecondaryColor, tempColor); //source
				break;
			}

			// MODE
			switch (effect._mMode)
			{
			case EChromaSDKSceneMode::SM_Max:
				colors[i] = MaxColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Min:
				colors[i] = MinColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Average:
				colors[i] = AverageColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Multiply:
				colors[i] = MultiplyColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Add:
				colors[i] = AddColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Subtract:
				colors[i] = SubtractColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Replace:
			default:
				if (color2 != 0) {
					colors[i] = color2;
				}
				break;
			}
		}
		deviceFrameIndex._mFrameIndex[device] = (frameId + frameCount + effect._mSpeed) % frameCount;
	}
}

void BlendAnimation2D(const FChromaSDKSceneEffect& effect, FChromaSDKDeviceFrameIndex& deviceFrameIndex, int device, EChromaSDKDevice2DEnum device2D, const char* animationName,
	int* colors, int* tempColors)
{
	const int size = GetColorArraySize2D(device2D);
	const int frameId = deviceFrameIndex._mFrameIndex[device];
	const int frameCount = ChromaAnimationAPI::GetFrameCountName(animationName);
	if (frameId < frameCount)
	{
		//cout << animationName << ": " << (1 + frameId) << " of " << frameCount << endl;
		float duration;
		int animationId = ChromaAnimationAPI::GetAnimation(animationName);
		ChromaAnimationAPI::GetFrame(animationId, frameId, &duration, tempColors, size);
		for (int i = 0; i < size; ++i)
		{
			int color1 = colors[i]; //target
			int tempColor = tempColors[i]; //source

			// BLEND
			int color2;
			switch (effect._mBlend)
			{
			case EChromaSDKSceneBlend::SB_None:
				color2 = tempColor; //source
				break;
			case EChromaSDKSceneBlend::SB_Invert:
				if (tempColor != 0) //source
				{
					color2 = InvertColor(tempColor); //source inverted
				}
				else
				{
					color2 = 0;
				}
				break;
			case EChromaSDKSceneBlend::SB_Threshold:
				color2 = Thresh(effect._mPrimaryColor, effect._mSecondaryColor, tempColor); //source
				break;
			case EChromaSDKSceneBlend::SB_Lerp:
			default:
				color2 = MultiplyNonZeroTargetColorLerp(effect._mPrimaryColor, effect._mSecondaryColor, tempColor); //source
				break;
			}

			// MODE
			switch (effect._mMode)
			{
			case EChromaSDKSceneMode::SM_Max:
				colors[i] = MaxColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Min:
				colors[i] = MinColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Average:
				colors[i] = AverageColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Multiply:
				colors[i] = MultiplyColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Add:
				colors[i] = AddColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Subtract:
				colors[i] = SubtractColor(color1, color2);
				break;
			case EChromaSDKSceneMode::SM_Replace:
			default:
				if (color2 != 0) {
					colors[i] = color2;
				}
				break;
			}
		}
		deviceFrameIndex._mFrameIndex[device] = (frameId + frameCount + effect._mSpeed) % frameCount;
	}
}

void BlendAnimations(FChromaSDKScene& scene,
	int* colorsChromaLink, int* tempColorsChromaLink,
	int* colorsHeadset, int* tempColorsHeadset,
	int* colorsKeyboard, int* tempColorsKeyboard,
	int* colorsKeypad, int* tempColorsKeypad,
	int* colorsMouse, int* tempColorsMouse,
	int* colorsMousepad, int* tempColorsMousepad)
{
	// blend active animations
	vector<FChromaSDKSceneEffect>& effects = scene._mEffects;
	for (vector<FChromaSDKSceneEffect>::iterator iter = effects.begin(); iter != effects.end(); ++iter)
	{
		FChromaSDKSceneEffect& effect = *iter;
		if (effect._mState)
		{
			FChromaSDKDeviceFrameIndex& deviceFrameIndex = effect._mFrameIndex;

			//iterate all device types
			for (int d = (int)EChromaSDKDeviceEnum::DE_ChromaLink; d < (int)EChromaSDKDeviceEnum::DE_MAX; ++d)
			{
				string animationName = effect._mAnimation;

				switch ((EChromaSDKDeviceEnum)d)
				{
				case EChromaSDKDeviceEnum::DE_ChromaLink:
					animationName += "_ChromaLink.chroma";
					BlendAnimation1D(effect, deviceFrameIndex, d, EChromaSDKDevice1DEnum::DE_ChromaLink, animationName.c_str(), colorsChromaLink, tempColorsChromaLink);
					break;
				case EChromaSDKDeviceEnum::DE_Headset:
					animationName += "_Headset.chroma";
					BlendAnimation1D(effect, deviceFrameIndex, d, EChromaSDKDevice1DEnum::DE_Headset, animationName.c_str(), colorsHeadset, tempColorsHeadset);
					break;
				case EChromaSDKDeviceEnum::DE_Keyboard:
					animationName += "_Keyboard.chroma";
					BlendAnimation2D(effect, deviceFrameIndex, d, EChromaSDKDevice2DEnum::DE_Keyboard, animationName.c_str(), colorsKeyboard, tempColorsKeyboard);
					break;
				case EChromaSDKDeviceEnum::DE_Keypad:
					animationName += "_Keypad.chroma";
					BlendAnimation2D(effect, deviceFrameIndex, d, EChromaSDKDevice2DEnum::DE_Keypad, animationName.c_str(), colorsKeypad, tempColorsKeypad);
					break;
				case EChromaSDKDeviceEnum::DE_Mouse:
					animationName += "_Mouse.chroma";
					BlendAnimation2D(effect, deviceFrameIndex, d, EChromaSDKDevice2DEnum::DE_Mouse, animationName.c_str(), colorsMouse, tempColorsMouse);
					break;
				case EChromaSDKDeviceEnum::DE_Mousepad:
					animationName += "_Mousepad.chroma";
					BlendAnimation1D(effect, deviceFrameIndex, d, EChromaSDKDevice1DEnum::DE_Mousepad, animationName.c_str(), colorsMousepad, tempColorsMousepad);
					break;
				}
			}
		}
	}
}

void GameLoop()
{
	const int sizeChromaLink = GetColorArraySize1D(EChromaSDKDevice1DEnum::DE_ChromaLink);
	const int sizeHeadset = GetColorArraySize1D(EChromaSDKDevice1DEnum::DE_Headset);
	const int sizeKeyboard = GetColorArraySize2D(EChromaSDKDevice2DEnum::DE_Keyboard);
	const int sizeKeypad = GetColorArraySize2D(EChromaSDKDevice2DEnum::DE_Keypad);
	const int sizeMouse = GetColorArraySize2D(EChromaSDKDevice2DEnum::DE_Mouse);
	const int sizeMousepad = GetColorArraySize1D(EChromaSDKDevice1DEnum::DE_Mousepad);

	int* colorsChromaLink = new int[sizeChromaLink];
	int* colorsHeadset = new int[sizeHeadset];
	int* colorsKeyboard = new int[sizeKeyboard];
	int* colorsKeypad = new int[sizeKeypad];
	int* colorsMouse = new int[sizeMouse];
	int* colorsMousepad = new int[sizeMousepad];

	int* tempColorsChromaLink = new int[sizeChromaLink];
	int* tempColorsHeadset = new int[sizeHeadset];
	int* tempColorsKeyboard = new int[sizeKeyboard];
	int* tempColorsKeypad = new int[sizeKeypad];
	int* tempColorsMouse = new int[sizeMouse];
	int* tempColorsMousepad = new int[sizeMousepad];

	unsigned int timeMS = 0;

	while (_sWaitForExit)
	{
		// start with a blank frame
		SetStaticColor(colorsChromaLink, _sAmbientColor, sizeChromaLink);
		SetStaticColor(colorsHeadset, _sAmbientColor, sizeHeadset);
		SetStaticColor(colorsKeyboard, _sAmbientColor, sizeKeyboard);
		SetStaticColor(colorsKeypad, _sAmbientColor, sizeKeypad);
		SetStaticColor(colorsMouse, _sAmbientColor, sizeMouse);
		SetStaticColor(colorsMousepad, _sAmbientColor, sizeMousepad);
		
#if !USE_ARRAY_EFFECTS

		SetupAnimation1D(ANIMATION_FINAL_CHROMA_LINK, EChromaSDKDevice1DEnum::DE_ChromaLink);
		SetupAnimation1D(ANIMATION_FINAL_HEADSET, EChromaSDKDevice1DEnum::DE_Headset);
		SetupAnimation2D(ANIMATION_FINAL_KEYBOARD, EChromaSDKDevice2DEnum::DE_Keyboard);
		SetupAnimation2D(ANIMATION_FINAL_KEYPAD, EChromaSDKDevice2DEnum::DE_Keypad);
		SetupAnimation2D(ANIMATION_FINAL_MOUSE, EChromaSDKDevice2DEnum::DE_Mouse);
		SetupAnimation1D(ANIMATION_FINAL_MOUSEPAD, EChromaSDKDevice1DEnum::DE_Mousepad);
		
#endif

		BlendAnimations(_sScene,
			colorsChromaLink, tempColorsChromaLink,
			colorsHeadset, tempColorsHeadset,
			colorsKeyboard, tempColorsKeyboard,
			colorsKeypad, tempColorsKeypad,
			colorsMouse, tempColorsMouse,
			colorsMousepad, tempColorsMousepad);

		if (_sAmmo)
		{
			// Show health animation
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

				float t = timeMS * 0.002f;
				float hp = fabsf(cos(MATH_PI / 2.0f + t));
				for (int i = 0; i < keysLength; ++i) {
					int color;
					if (((i + 1) / ((float)keysLength + 1)) < hp) {
						color = ChromaAnimationAPI::GetRGB(0, 255, 0);
					}
					else {
						color = ChromaAnimationAPI::GetRGB(0, 100, 0);
					}
					int key = keys[i];
					SetKeyColor(colorsKeyboard, key, color);
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

				float t = timeMS * 0.001f;
				float hp = fabsf(cos(MATH_PI / 2.0f + t));
				for (int i = 0; i < keysLength; ++i) {
					int color;
					if (((i + 1) / ((float)keysLength + 1)) < hp) {
						color = ChromaAnimationAPI::GetRGB(255, 255, 0);
					}
					else {
						color = ChromaAnimationAPI::GetRGB(100, 100, 0);
					}
					int key = keys[i];
					SetKeyColor(colorsKeyboard, key, color);
				}
			}
		}

		if (_sHotkeys)
		{
			// Show hotkeys
			SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_ESC, 255, 255, 0);
			SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_W, 255, 0, 0);
			SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_A, 255, 0, 0);
			SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_S, 255, 0, 0);
			SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_D, 255, 0, 0);

			if (_sAmmo)
			{
				SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_A, 0, 255, 0);
			}

			// Highlight R if rainbow is active
			if (_sScene._mEffects[_sIndexRainbow]._mState)
			{
				SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_R, 0, 255, 0);
			}

			// Highlight S if spiral is active
			if (_sScene._mEffects[_sIndexSpiral]._mState)
			{
				SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_S, 0, 255, 0);
			}

			// Highlight L if landscape is active
			if (_sScene._mEffects[_sIndexLandscape]._mState)
			{
				SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_L, 0, 255, 0);
			}

			// Highlight L if landscape is active
			if (_sScene._mEffects[_sIndexFire]._mState)
			{
				SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_F, 0, 255, 0);
			}

			if (_sHotkeys)
			{
				SetKeyColorRGB(colorsKeyboard, (int)Keyboard::RZKEY::RZKEY_H, 0, 255, 0);
			}
		}

#if USE_ARRAY_EFFECTS

		ChromaAnimationAPI::SetEffectCustom1D((int)EChromaSDKDevice1DEnum::DE_ChromaLink, colorsChromaLink);
		ChromaAnimationAPI::SetEffectCustom1D((int)EChromaSDKDevice1DEnum::DE_Headset, colorsHeadset);
		ChromaAnimationAPI::SetEffectCustom1D((int)EChromaSDKDevice1DEnum::DE_Mousepad, colorsMousepad);

		ChromaAnimationAPI::SetCustomColorFlag2D((int)EChromaSDKDevice2DEnum::DE_Keyboard, colorsKeyboard);
		ChromaAnimationAPI::SetEffectKeyboardCustom2D((int)EChromaSDKDevice2DEnum::DE_Keyboard, colorsKeyboard);

		ChromaAnimationAPI::SetEffectCustom2D((int)EChromaSDKDevice2DEnum::DE_Keypad, colorsKeypad);
		ChromaAnimationAPI::SetEffectCustom2D((int)EChromaSDKDevice2DEnum::DE_Mouse, colorsMouse);

#else

		ChromaAnimationAPI::UpdateFrameName(ANIMATION_FINAL_CHROMA_LINK, 0, 0.1f, colorsChromaLink, sizeChromaLink);
		ChromaAnimationAPI::UpdateFrameName(ANIMATION_FINAL_HEADSET, 0, 0.1f, colorsHeadset, sizeHeadset);
		ChromaAnimationAPI::UpdateFrameName(ANIMATION_FINAL_KEYBOARD, 0, 0.1f, colorsKeyboard, sizeKeyboard);
		ChromaAnimationAPI::UpdateFrameName(ANIMATION_FINAL_KEYPAD, 0, 0.1f, colorsKeypad, sizeKeypad);
		ChromaAnimationAPI::UpdateFrameName(ANIMATION_FINAL_MOUSE, 0, 0.1f, colorsMouse, sizeMouse);
		ChromaAnimationAPI::UpdateFrameName(ANIMATION_FINAL_MOUSEPAD, 0, 0.1f, colorsMousepad, sizeMousepad);

		// display the change
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL_CHROMA_LINK, 0);
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL_HEADSET, 0);
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL_KEYBOARD, 0);
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL_KEYPAD, 0);
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL_MOUSE, 0);
		ChromaAnimationAPI::PreviewFrameName(ANIMATION_FINAL_MOUSEPAD, 0);

#endif
		

		Sleep(33); //30 FPS
		timeMS += 33;
	}
	
	delete[] colorsChromaLink;
	delete[] colorsHeadset;
	delete[] colorsKeyboard;
	delete[] colorsKeypad;
	delete[] colorsMouse;
	delete[] colorsMousepad;

	delete[] tempColorsChromaLink;
	delete[] tempColorsHeadset;
	delete[] tempColorsKeyboard;
	delete[] tempColorsKeypad;
	delete[] tempColorsMouse;
	delete[] tempColorsMousepad;
}

void InputHandler()
{
	HandleInput inputEscape = HandleInput(VK_ESCAPE);
	HandleInput inputEnter = HandleInput(VK_RETURN);
	HandleInput inputUp = HandleInput(VK_UP);
	HandleInput inputDown = HandleInput(VK_DOWN);
	HandleInput inputA = HandleInput('A');
	HandleInput inputC = HandleInput('C');
	HandleInput inputH = HandleInput('H');
	HandleInput inputL = HandleInput('L');
	HandleInput inputF = HandleInput('F');
	HandleInput inputR = HandleInput('R');
	HandleInput inputS = HandleInput('S');

	bool inputDetected = true;

	int autoPrint = 0;
	while (_sWaitForExit)
	{
		if (++autoPrint > 100 || inputDetected)
		{
			autoPrint = 0;
			PrintLegend();
		}

		inputDetected = false;

		if (inputUp.WasReleased())
		{
			inputDetected = true;
			if (_sSelection > 0)
			{
				--_sSelection;
			}
		}

		if (inputDown.WasReleased())
		{
			inputDetected = true;
			if (_sSelection < MAX_SELECTION)
			{
				++_sSelection;
			}
		}

		if (inputEnter.WasReleased())
		{
			inputDetected = true;
			if (ChromaAnimationAPI::CoreStreamSupportsStreaming())
			{
				switch (_sSelection)
				{
				case 0:
					//ChromaAnimationAPI::CoreStreamGetAuthShortcode(_sShortcode, &_sLenShortcode, "PC", "Gameloop Sample App");
					ChromaAnimationAPI::CoreStreamGetAuthShortcode(_sShortcode, &_sLenShortcode, "GEFORCE_NOW", "Gameloop Cloud Sample App");
					break;
				case 1:
					ChromaAnimationAPI::CoreStreamGetId(_sShortcode, _sStreamId, &_sLenStreamId);
					break;
				case 2:
					ChromaAnimationAPI::CoreStreamGetKey(_sShortcode, _sStreamKey, &_sLenStreamKey);
					break;
				case 3:
					if (ChromaAnimationAPI::CoreStreamReleaseShortcode(_sShortcode))
					{
						memset(_sShortcode, 0, size(_sShortcode));
						_sLenShortcode = 0;
					}
					break;
				case 4:
					ChromaAnimationAPI::CoreStreamBroadcast(_sStreamId, _sStreamKey);
					break;
				case 5:
					ChromaAnimationAPI::CoreStreamBroadcastEnd();
					break;
				case 6:
					ChromaAnimationAPI::CoreStreamWatch(_sStreamId, 0);
					break;
				case 7:
					ChromaAnimationAPI::CoreStreamWatchEnd();
					break;
				}
			}
		}

		if (inputEscape.WasReleased())
		{
			inputDetected = true;
			_sWaitForExit = false;
		}
		if (inputA.WasReleased())
		{
			inputDetected = true;
			_sAmmo = !_sAmmo;
		}
		if (inputH.WasReleased())
		{
			inputDetected = true;
			_sHotkeys = !_sHotkeys;
		}
		if (inputC.WasReleased())
		{
			inputDetected = true;
			_sAmbientColor = ChromaAnimationAPI::GetRGB(rand() % 256, rand() % 256, rand() % 256);
		}
		if (inputF.WasReleased())
		{
			inputDetected = true;
			_sScene.ToggleState(_sIndexFire);
			_sAmbientColor = 0;
		}
		if (inputL.WasReleased())
		{
			inputDetected = true;
			_sScene.ToggleState(_sIndexLandscape);
			_sAmbientColor = 0;
		}
		if (inputR.WasReleased())
		{
			inputDetected = true;
			_sScene.ToggleState(_sIndexRainbow);
			_sAmbientColor = 0;
		}
		if (inputS.WasReleased())
		{
			inputDetected = true;
			_sScene.ToggleState(_sIndexSpiral);
			_sAmbientColor = 0;
		}

		Sleep(1);
	}
}

void Cleanup()
{
	ChromaAnimationAPI::StopAll();
	ChromaAnimationAPI::CloseAll();
	RZRESULT result = ChromaAnimationAPI::Uninit();
	ChromaAnimationAPI::UnloadLibrarySDK();
	if (result != RZRESULT_SUCCESS)
	{
		cerr << "Failed to uninitialize Chroma!" << endl;
		exit(1);
	}
}

int main()
{
	fprintf(stdout, "C++ GAME LOOP CHROMA SAMPLE APP\r\n\r\n");

	// setup scene
	_sScene = FChromaSDKScene();

	FChromaSDKSceneEffect effect = FChromaSDKSceneEffect();
	effect._mAnimation = "Animations/Landscape";
	effect._mSpeed = 1;
	effect._mBlend = EChromaSDKSceneBlend::SB_None;
	effect._mState = false;
	effect._mMode = EChromaSDKSceneMode::SM_Add;
	_sScene._mEffects.push_back(effect);
	_sIndexLandscape = (int)_sScene._mEffects.size() - 1;

	effect = FChromaSDKSceneEffect();
	effect._mAnimation = "Animations/Fire";
	effect._mSpeed = 1;
	effect._mBlend = EChromaSDKSceneBlend::SB_None;
	effect._mState = false;
	effect._mMode = EChromaSDKSceneMode::SM_Add;
	_sScene._mEffects.push_back(effect);
	_sIndexFire = (int)_sScene._mEffects.size() - 1;

	effect = FChromaSDKSceneEffect();
	effect._mAnimation = "Animations/Rainbow";
	effect._mSpeed = 1;
	effect._mBlend = EChromaSDKSceneBlend::SB_None;
	effect._mState = true;
	effect._mMode = EChromaSDKSceneMode::SM_Add;
	_sScene._mEffects.push_back(effect);
	_sIndexRainbow = (int)_sScene._mEffects.size() - 1;

	effect = FChromaSDKSceneEffect();
	effect._mAnimation = "Animations/Spiral";
	effect._mSpeed = 1;
	effect._mBlend = EChromaSDKSceneBlend::SB_None;
	effect._mState = false;
	effect._mMode = EChromaSDKSceneMode::SM_Add;
	_sScene._mEffects.push_back(effect);
	_sIndexSpiral = (int)_sScene._mEffects.size() - 1;

	Init();

	thread thread(GameLoop);

	InputHandler();

	thread.join();
	Cleanup();
	return 0;
}
