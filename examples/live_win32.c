/* Example of simple PocketSphinx speech segmentation.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
/**
 * @example live_win32.c
 * @brief Speech recognition with live audio input and endpointing.
 *
 * This file shows how to use PocketSphinx with microphone input using
 * the Win32 wave API (the only one of many terrible audio APIs on
 * Windows that isn't made even more terrible by requiring you to use
 * C++ in an unmanaged environment).
 *
 * To build it, you should be able to find a "live_win32" target in
 * your favorite IDE after running CMake - in Visual Studio Code, look
 * in the "CMake" tab.
 */

#include <windows.h>
#include <mmsystem.h>
#include <pocketsphinx.h>
#include <signal.h>

#define CHECK(expr)                                          \
    do                                                       \
    {                                                        \
        char errbuf[MAXERRORLENGTH];                         \
        int err;                                             \
        if ((err = expr) != 0)                               \
        {                                                    \
            waveInGetErrorText(err, errbuf, sizeof(errbuf)); \
            E_FATAL("error %08x: %s\n", err, errbuf);        \
        }                                                    \
    } while (0)

int main(int argc, char *argv[])
{
    int err;
    HWAVEIN wavein;
    WAVEFORMATEX wavefmt;
    HANDLE event;
    WAVEHDR hdrs[32];
    int i;
    size_t bufsize;

    err_set_loglevel(ERR_INFO);

    wavefmt.wFormatTag = WAVE_FORMAT_PCM;
    wavefmt.nChannels = 1;
    wavefmt.nSamplesPerSec = 16000;
    wavefmt.wBitsPerSample = 16;
    wavefmt.nBlockAlign = 2;
    wavefmt.nAvgBytesPerSec = wavefmt.nSamplesPerSec * wavefmt.nBlockAlign;
    wavefmt.cbSize = 0;

    event = CreateEventA(NULL, TRUE, FALSE, "buffer_ready");
    CHECK(waveInOpen(&wavein, WAVE_MAPPER, &wavefmt,
                     (DWORD_PTR)event, 0, CALLBACK_EVENT));
    memset(hdrs, 0, sizeof(hdrs));
    bufsize = 1920;
    for (i = 0; i < 32; ++i)
    {
        hdrs[i].lpData = malloc(bufsize);
        hdrs[i].dwBufferLength = (DWORD)bufsize;
        CHECK(waveInPrepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
        CHECK(waveInAddBuffer(wavein, &hdrs[i], sizeof(hdrs[i])));
    }
    CHECK(waveInStart(wavein));
    i = 0;
    while (1)
    {
        WaitForSingleObject(event, INFINITE);
        if (hdrs[i].dwFlags & WHDR_DONE)
        {
            E_INFO("Buffer %d got %lu bytes\n",
                   i, hdrs[i].dwBytesRecorded);
            CHECK(waveInUnprepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
            CHECK(waveInPrepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
            CHECK(waveInAddBuffer(wavein, &hdrs[i], sizeof(hdrs[i])));
            if (++i == 32)
                i = 0;
            ResetEvent(event);
        }
    }

    return 0;
}
