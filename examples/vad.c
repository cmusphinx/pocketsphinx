/* Example of simple PocketSphinx voice activity detection.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
#include <pocketsphinx.h>
#include <signal.h>

static int global_done = 0;
void
catch_sig(int signum)
{
    (void)signum;
    global_done = 1;
}

int
main(int argc, char *argv[])
{
    ps_vad_t *vader;
    char *soxcmd;
    FILE *sox;
    short *frame;
    size_t frame_size;
    int len, prev_is_speech;

    (void)argc; (void)argv;
    if ((vader = ps_vad_init(0, 0, 0)) == NULL)
        E_FATAL("PocketSphinx VAD init failed\n");
    #define SOXCMD "sox -q -r %d -c 1 -b 16 -e signed-integer -d -t raw -"
    len = snprintf(NULL, 0, SOXCMD, ps_vad_sample_rate(vader));
    if ((soxcmd = malloc(len + 1)) == NULL)
        E_FATAL_SYSTEM("Failed to allocate string");
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    if (snprintf(soxcmd, len + 1, SOXCMD, ps_vad_sample_rate(vader)) != len)
        E_FATAL_SYSTEM("snprintf() failed");
    if ((sox = popen(soxcmd, "r")) == NULL)
        E_FATAL_SYSTEM("Failed to popen(%s)", soxcmd);
    free(soxcmd);
    frame_size = ps_vad_frame_size(vader);
    if ((frame = malloc(frame_size * sizeof(frame[0]))) == NULL)
        E_FATAL_SYSTEM("Failed to allocate frame");
    prev_is_speech = 0;
    while (!global_done) {
        int is_speech;
        if (fread(frame, sizeof(frame[0]), frame_size, sox) != frame_size) {
            if (!feof(sox))
                E_ERROR_SYSTEM("Failed to read %d samples", frame_size);
            break;
        }
        is_speech = ps_vad_classify(vader, frame);
        if (is_speech != prev_is_speech)
            printf("%s\n", is_speech ? "speech" : "not speech");
        prev_is_speech = is_speech;
    }
    free(frame);
    if (pclose(sox) < 0)
        E_ERROR_SYSTEM("Failed to pclose(sox)");
    ps_vad_free(vader);
        
    return 0;
}
