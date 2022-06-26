#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
// new, remove if possible
#include <process.h>

#include "DDRIO.h"

#pragma comment (lib, "Setupapi.lib")

typedef int(*thread_create_t)(int(*proc)(void*), void* ctx, unsigned int stack_sz, unsigned int priority);
typedef void(*thread_join_t)(int thread_id, int* result);
typedef void(*thread_destroy_t)(int thread_id);

typedef bool(*DDRIO_IO_INIT)(thread_create_t thread_create, thread_join_t thread_join, thread_destroy_t thread_destroy);
static DDRIO_IO_INIT ddrio_io_init;

typedef int(*DDRIO_READ_PAD)();
static DDRIO_READ_PAD ddrio_io_read_pad;

typedef int(*DDRIO_SETLIGHTS_P3IO)(unsigned int lights);
static DDRIO_SETLIGHTS_P3IO ddrio_set_lights_p3io;

typedef int(*DDRIO_SETLIGHTS_EXTIO)(unsigned int lights);
static DDRIO_SETLIGHTS_EXTIO ddrio_set_lights_extio;

typedef int(*DDRIO_SETLIGHTS_HDXSPANEL)(unsigned int lights);
static DDRIO_SETLIGHTS_HDXSPANEL ddrio_set_lights_hdxs_panel;

typedef int(*DDRIO_SETLIGHTS_HDXSRGB)(unsigned char idx, unsigned char r, unsigned char g, unsigned char b);
static DDRIO_SETLIGHTS_HDXSRGB ddrio_set_lights_hdxs_rgb;

typedef int(*DDRIO_FINI)();
static DDRIO_FINI ddrio_fini;



struct shim_ctx {
	HANDLE barrier;
	int(*proc)(void*);
	void* ctx;
};

static unsigned int crt_thread_shim(void* outer_ctx)
{
	// printf("crt_thread_shim");

	struct shim_ctx* sctx = (struct shim_ctx*)outer_ctx;
	int(*proc)(void*);
	void* inner_ctx;

	proc = sctx->proc;
	inner_ctx = sctx->ctx;

	SetEvent(sctx->barrier);

	return proc(inner_ctx);
}


int crt_thread_create(
	int(*proc)(void*), void* ctx, unsigned int stack_sz, unsigned int priority)
{

	// printf("crt_thread_create");

	struct shim_ctx sctx;
	uintptr_t thread_id;

	sctx.barrier = CreateEvent(NULL, TRUE, FALSE, NULL);
	sctx.proc = proc;
	sctx.ctx = ctx;

	thread_id = _beginthreadex(NULL, stack_sz, (_beginthreadex_proc_type)crt_thread_shim, &sctx, 0, NULL);

	WaitForSingleObject(sctx.barrier, INFINITE);
	CloseHandle(sctx.barrier);

	// printf("crt_thread_create %d", thread_id);

	return (int)thread_id;

}

void crt_thread_destroy(int thread_id)
{
	// printf("crt_thread_destroy %d", thread_id);

	CloseHandle((HANDLE)(uintptr_t)thread_id);
}


void crt_thread_join(int thread_id, int* result)
{
	// printf("crt_thread_join %d", thread_id);

	WaitForSingleObject((HANDLE)(uintptr_t)thread_id, INFINITE);

	if (result)
	{
		GetExitCodeThread((HANDLE)(uintptr_t)thread_id, (DWORD*)result);
	}
}


DDRIO::DDRIO(_TCHAR *ddrio_dll_filename)
{
    /* Start with not being ready */
    is_ready = false;

	
	ddrio_dll = LoadLibrary(ddrio_dll_filename);

	if (!ddrio_dll)
	{
		fprintf(stderr, "Failed to find ddrio DLL! Is your path correct?\n");
		return;
	}

	//pull the functions we need out of the dll.
	ddrio_io_init = (DDRIO_IO_INIT)GetProcAddress(ddrio_dll, "ddr_io_init");
	ddrio_io_read_pad = (DDRIO_READ_PAD)GetProcAddress(ddrio_dll, "ddr_io_read_pad");
	ddrio_fini = (DDRIO_FINI)GetProcAddress(ddrio_dll, "ddr_io_fini");

	//these are technically optional depending on the device in question.
	ddrio_set_lights_p3io = (DDRIO_SETLIGHTS_P3IO)GetProcAddress(ddrio_dll, "ddr_io_set_lights_p3io");
	ddrio_set_lights_extio = (DDRIO_SETLIGHTS_EXTIO)GetProcAddress(ddrio_dll, "ddr_io_set_lights_extio");
	ddrio_set_lights_hdxs_panel = (DDRIO_SETLIGHTS_HDXSPANEL)GetProcAddress(ddrio_dll, "ddr_io_set_lights_hdxs_panel");
	ddrio_set_lights_hdxs_rgb = (DDRIO_SETLIGHTS_HDXSRGB)GetProcAddress(ddrio_dll, "ddr_io_set_lights_hdxs_rgb");

	if (!ddrio_io_init || !ddrio_io_read_pad || !ddrio_fini)
	{
		fprintf(stderr, "Required ddrio functions not found! Is this the right DLL?\n");
		return;
	}

    /* Now, perform init sequence */
	thread_create_t thread_impl_create = crt_thread_create;
	thread_join_t thread_impl_join = crt_thread_join;
	thread_destroy_t thread_impl_destroy = crt_thread_destroy;

	if (!ddrio_io_init(thread_impl_create, thread_impl_join, thread_impl_destroy))
	{
		fprintf(stderr, "Failed to init ddrio! Did you set up its dependencies?\n");
		if (ddrio_fini)
		{
			ddrio_fini();
		}
		return;
	}


	/* Looks good! */
	is_ready = true;
	sequence = 0;
	buttons = 0;
	lastButtons = 0;
	lastpadlights = 0xFFFFFFFF;
	lastcablights = 0xFFFFFFFF;
	lasthdxslights = 0xFFFFFFFF;
	lasthdxsrgb = 0xFFFFFFFF;

    SetLights(0);
	SetLightsRGB(0, 0, 0);
}

DDRIO::~DDRIO()
{
    // if (!is_ready) { return; }

	SetLights(0);
	SetLightsRGB(0, 0, 0);

    // Kill the handle to ddrio
	if (ddrio_fini) ddrio_fini();
	if (ddrio_dll) FreeLibrary(ddrio_dll);

    is_ready = false;
}

bool DDRIO::Ready()
{
    return is_ready;
}

void DDRIO::SetLights(unsigned int lights)
{
    /* Mask off the lights bits for each part */
    unsigned int p3iolights = lights & (
        (1 << LIGHT_1P_MENU) |
		(1 << LIGHT_2P_MENU) |
		(1 << LIGHT_MARQUEE_LOWER_RIGHT) |
		(1 << LIGHT_MARQUEE_UPPER_RIGHT) |
		(1 << LIGHT_MARQUEE_LOWER_LEFT) |
		(1 << LIGHT_MARQUEE_UPPER_LEFT)
    );
    unsigned int extiolights = lights & (
		(1 << LIGHT_1P_RIGHT) |
		(1 << LIGHT_1P_LEFT) |
		(1 << LIGHT_1P_DOWN) |
		(1 << LIGHT_1P_UP) |
		(1 << LIGHT_2P_RIGHT) |
		(1 << LIGHT_2P_LEFT) |
		(1 << LIGHT_2P_DOWN) |
		(1 << LIGHT_2P_UP) |
		(1 << LIGHT_BASS_NEONS)
    );
	unsigned int hdxslights = (
		(((p3iolights >> LIGHT_1P_MENU) & 1) << LIGHT_HD_1P_START) |
		(((p3iolights >> LIGHT_1P_MENU) & 1) << LIGHT_HD_1P_UP_DOWN) |
		(((p3iolights >> LIGHT_1P_MENU) & 1) << LIGHT_HD_1P_LEFT_RIGHT) |
		(((p3iolights >> LIGHT_2P_MENU) & 1) << LIGHT_HD_2P_START) |
		(((p3iolights >> LIGHT_2P_MENU) & 1) << LIGHT_HD_2P_UP_DOWN) |
		(((p3iolights >> LIGHT_2P_MENU) & 1) << LIGHT_HD_2P_LEFT_RIGHT)
	);

    if (lastcablights != p3iolights)
    {
		if (ddrio_set_lights_p3io) ddrio_set_lights_p3io(p3iolights);
        lastcablights = p3iolights;
    }

    if (lastpadlights != extiolights)
    {
		if (ddrio_set_lights_extio) ddrio_set_lights_extio(extiolights);
        lastpadlights = extiolights;
    }

	if (lasthdxslights != hdxslights)
	{
		if (ddrio_set_lights_hdxs_panel) ddrio_set_lights_hdxs_panel(hdxslights);
		lasthdxslights = hdxslights;
	}

}

void DDRIO::SetLightsRGB(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int hdxsrgb = (r << 16) | (g << 8) | b;

	if (lasthdxsrgb != hdxsrgb)
	{
		if (ddrio_set_lights_hdxs_rgb)
		{
			for (unsigned char idx = 0; idx < 4; idx++)
			{
				ddrio_set_lights_hdxs_rgb(idx, r, g, b);
			}
		}

		lasthdxslights = hdxsrgb;
	}
}

unsigned int DDRIO::GetButtonsHeld()
{
    if (!is_ready)
    {
        return 0;
    }

	return ddrio_io_read_pad();
}

void DDRIO::Tick()
{
    if (!is_ready) { return; }

    // Remember last buttons so we can calculate newly
    // pressed buttons.
    lastButtons = buttons;

    // Poll the JAMMA edge to get current held buttons
    buttons = GetButtonsHeld();
}

unsigned int DDRIO::ButtonsPressed()
{
    // Return only buttons pressed in the last Tick() operation.
    return buttons & (~lastButtons);
}

bool DDRIO::ButtonPressed(unsigned int button)
{
    // Return whether the current button mask is pressed.
    return (ButtonsPressed() & (1 << button)) != 0;
}

unsigned int DDRIO::ButtonsHeld()
{
    // Return all buttons currently held down.
    return buttons;
}

bool DDRIO::ButtonHeld(unsigned int button)
{
    // Return whether the current button mask is held.
    return (ButtonsHeld() & (1 << button)) != 0;
}

void DDRIO::LightOn(unsigned int light)
{
    SetLights((lastcablights | lastpadlights) | light);
}

void DDRIO::LightOff(unsigned int light)
{
    SetLights((lastcablights | lastpadlights) & (~light));
}

/*
 * (c) 2022 din, Fishaman P
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
 