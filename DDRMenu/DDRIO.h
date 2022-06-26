#pragma once

#include <tchar.h>
#include <windows.h>


// Button definitions
enum ddr_pad_bit {
	BUTTON_TEST = 0x04,
	BUTTON_COIN = 0x05,
	BUTTON_SERVICE = 0x06,

	BUTTON_2P_START = 0x08,
	BUTTON_2P_UP = 0x09,
	BUTTON_2P_DOWN = 0x0A,
	BUTTON_2P_LEFT = 0x0B,
	BUTTON_2P_RIGHT = 0x0C,
	BUTTON_2P_MENULEFT = 0x0E,
	BUTTON_2P_MENURIGHT = 0x0F,
	BUTTON_2P_MENUUP = 0x02,
	BUTTON_2P_MENUDOWN = 0x03,

	BUTTON_1P_START = 0x10,
	BUTTON_1P_UP = 0x11,
	BUTTON_1P_DOWN = 0x12,
	BUTTON_1P_LEFT = 0x13,
	BUTTON_1P_RIGHT = 0x14,
	BUTTON_1P_MENULEFT = 0x16,
	BUTTON_1P_MENURIGHT = 0x17,
	BUTTON_1P_MENUUP = 0x00,
	BUTTON_1P_MENUDOWN = 0x01,
};


// Lights definitions
enum p3io_light_bit {
	LIGHT_1P_MENU = 0x00,
	LIGHT_2P_MENU = 0x01,
	LIGHT_MARQUEE_LOWER_RIGHT = 0x04,
	LIGHT_MARQUEE_UPPER_RIGHT = 0x05,
	LIGHT_MARQUEE_LOWER_LEFT = 0x06,
	LIGHT_MARQUEE_UPPER_LEFT = 0x07
};

enum extio_light_bit {
	LIGHT_BASS_NEONS = 0x0E,

	LIGHT_2P_RIGHT = 0x13,
	LIGHT_2P_LEFT = 0x14,
	LIGHT_2P_DOWN = 0x15,
	LIGHT_2P_UP = 0x16,

	LIGHT_1P_RIGHT = 0x1B,
	LIGHT_1P_LEFT = 0x1C,
	LIGHT_1P_DOWN = 0x1D,
	LIGHT_1P_UP = 0x1E
};

enum hdxs_light_bit {
	LIGHT_HD_1P_START = 0x08,
	LIGHT_HD_1P_UP_DOWN = 0x09,
	LIGHT_HD_1P_LEFT_RIGHT = 0x0A,
	LIGHT_HD_2P_START = 0x0B,
	LIGHT_HD_2P_UP_DOWN = 0x0C,
	LIGHT_HD_2P_LEFT_RIGHT = 0x0D,
};

enum hdxs_rgb_light_idx {
	LIGHT_HD_1P_SPEAKER_F_R = 0x00,
	LIGHT_HD_1P_SPEAKER_F_G = 0x01,
	LIGHT_HD_1P_SPEAKER_F_B = 0x02,
	LIGHT_HD_2P_SPEAKER_F_R = 0x03,
	LIGHT_HD_2P_SPEAKER_F_G = 0x04,
	LIGHT_HD_2P_SPEAKER_F_B = 0x05,
	LIGHT_HD_1P_SPEAKER_W_R = 0x06,
	LIGHT_HD_1P_SPEAKER_W_G = 0x07,
	LIGHT_HD_1P_SPEAKER_W_B = 0x08,
	LIGHT_HD_2P_SPEAKER_W_R = 0x09,
	LIGHT_HD_2P_SPEAKER_W_G = 0x0A,
	LIGHT_HD_2P_SPEAKER_W_B = 0x0B,
};


class DDRIO
{
public:
    DDRIO(_TCHAR *ddrio_dll_filename);
    ~DDRIO();

    bool Ready();
    void Tick();
    unsigned int ButtonsPressed();
    bool ButtonPressed(unsigned int button);
    unsigned int ButtonsHeld();
    bool ButtonHeld(unsigned int button);
    void SetLights(unsigned int lights);
	void SetLightsRGB(unsigned char r, unsigned char g, unsigned char b);
    void LightOn(unsigned int light);
    void LightOff(unsigned int light);
private:
	HINSTANCE ddrio_dll;

    unsigned int GetButtonsHeld();

    bool is_ready;
    unsigned int sequence;
    unsigned int buttons;
    unsigned int lastButtons;
    unsigned int lastcablights;
    unsigned int lastpadlights;
	unsigned int lasthdxslights;
	unsigned int lasthdxsrgb;
};
