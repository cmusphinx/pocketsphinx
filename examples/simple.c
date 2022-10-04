/* Example of simple PocketSphinx recognition.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
/**
 * @example simple.c
 * @brief Simplest possible example of speech recognition in C.
 *
 * This file shows how to use PocketSphinx to recognize a single input
 * file.  To compile it, assuming you have built the library as in
 * \ref unix_install "these directions", you can run:
 *
 *     cmake --build build --target simple
 *
 * Alternately, if PocketSphinx is installed system-wide, you can run:
 *
 *     gcc -o simple simple.c $(pkg-config --libs --cflags pocketsphinx)
 *
 *
 */

#include <pocketsphinx.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    ps_decoder_t *decoder;
    ps_config_t *config;
    FILE *fh;
    short *buf;
    size_t len, nsamples;

    /* Look for a single audio file as input parameter. */
    if (argc < 2)
        E_FATAL("Usage: %s FILE\n");
    if ((fh = fopen(argv[1], "rb")) == NULL)
        E_FATAL_SYSTEM("Failed to open %s", argv[1]);

    /* Get the size of the input. */
    if (fseek(fh, 0, SEEK_END) < 0)
        E_FATAL_SYSTEM("Unable to find end of input file %s", argv[1]);
    len = ftell(fh);
    rewind(fh);

    /* Initialize configuration from input file. */
    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if (ps_config_soundfile(config, fh, argv[1]) < 0)
        E_FATAL("Unsupported input file %s\n", argv[1]);
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");

    /* Allocate data (skipping header) */
    len -= ftell(fh);
    if ((buf = malloc(len)) == NULL)
        E_FATAL_SYSTEM("Unable to allocate %d bytes", len);
    /* Read input */
    nsamples = fread(buf, sizeof(buf[0]), len / sizeof(buf[0]), fh);
    if (nsamples != len / sizeof(buf[0]))
        E_FATAL_SYSTEM("Unable to read %d samples", len / sizeof(buf[0]));

    /* Recognize it! */
    if (ps_start_utt(decoder) < 0)
        E_FATAL("Failed to start processing\n");
    if (ps_process_raw(decoder, buf, nsamples, FALSE, TRUE) < 0)
        E_FATAL("ps_process_raw() failed\n");
    if (ps_end_utt(decoder) < 0)
        E_FATAL("Failed to end processing\n");

    /* Print the result */
    if (ps_get_hyp(decoder, NULL) != NULL)
        printf("%s\n", ps_get_hyp(decoder, NULL));

    /* Clean up */
    if (fclose(fh) < 0)
        E_FATAL_SYSTEM("Failed to close %s", argv[1]);
    free(buf);
    ps_free(decoder);
    ps_config_free(config);
        
    return 0;
}
