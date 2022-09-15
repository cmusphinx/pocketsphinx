/* Example of simple PocketSphinx recognition.
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
    ps_decoder_t *decoder;
    ps_config_t *config;
    char *soxcmd;
    FILE *sox;
    #define BUFLEN 4096 // about 250ms
    short buf[BUFLEN];
    size_t len;

    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");
    #define SOXCMD "sox -q -r %ld -c 1 -b 16 -e signed-integer -d -t raw -"
    len = snprintf(NULL, 0, SOXCMD,
                   ps_config_int(config, "samprate"));
    if ((soxcmd = malloc(len + 1)) == NULL)
        E_FATAL_SYSTEM("Failed to allocate string");
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    if (snprintf(soxcmd, len + 1, SOXCMD,
                 ps_config_int(config, "samprate")) != len)
        E_FATAL_SYSTEM("snprintf() failed");
    if ((sox = popen(soxcmd, "r")) == NULL)
        E_FATAL_SYSTEM("Failed to popen(%s)", soxcmd);
    free(soxcmd);
    ps_start_utt(decoder);
    while (!global_done) {
        const char *hyp;
        if ((len = fread(buf, sizeof(buf[0]), BUFLEN, sox)) == 0)
            break;
        if (ps_process_raw(decoder, buf, len, FALSE, FALSE) < 0)
            E_FATAL("ps_process_raw() failed\n");
        if ((hyp = ps_get_hyp(decoder, NULL)) != NULL)
            printf("%s\n", hyp);
    }
    ps_end_utt(decoder);
    if (pclose(sox) < 0)
        E_ERROR_SYSTEM("Failed to pclose(sox)");
    if (ps_get_hyp(decoder, NULL) != NULL)
        printf("%s\n", ps_get_hyp(decoder, NULL));
    ps_free(decoder);
    ps_config_free(config);
        
    return 0;
}
