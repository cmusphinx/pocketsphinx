#include <stdio.h>
#include <string.h>

#include "fe/fe.h"
#include "util/cmd_ln.h"
#include "util/ckd_alloc.h"
#include <pocketsphinx/err.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    static const ps_arg_t fe_args[] = {
        waveform_to_cepstral_command_line_macro(),
        { NULL, 0, NULL, NULL }
    };
    FILE *raw;
    cmd_ln_t *config;
    fe_t *fe;
    int16 buf[1024];
    int16 const *inptr;
    int32 frame_shift, frame_size;
    mfcc_t **cepbuf1, **cepbuf2, **cptr;
    int32 nfr, i;
    size_t nsamp;

    err_set_loglevel_str("INFO");
    TEST_ASSERT(config = cmd_ln_parse_r(NULL, fe_args, argc, argv, FALSE));
    TEST_ASSERT(fe = fe_init_auto_r(config));

    TEST_EQUAL(fe_get_output_size(fe), DEFAULT_NUM_CEPSTRA);

    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(frame_shift, DEFAULT_FRAME_SHIFT);
    TEST_EQUAL(frame_size, (int)(DEFAULT_WINDOW_LENGTH*DEFAULT_SAMPLING_RATE));

    TEST_ASSERT(raw = fopen(TESTDATADIR "/chan3.raw", "rb"));

    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_EQUAL(1024, fread(buf, sizeof(int16), 1024, raw));

    nsamp = 1024;
    TEST_ASSERT(fe_process_frames(fe, NULL, &nsamp, NULL, &nfr) >= 0);
    TEST_EQUAL(1024, nsamp);
    TEST_EQUAL(4, nfr);

    cepbuf1 = ckd_calloc_2d(5, DEFAULT_NUM_CEPSTRA, sizeof(**cepbuf1));
    inptr = &buf[0];
    nfr = 1;

    /* Process the data, one frame at a time. */
    E_INFO("Testing one frame at a time (1024 samples)\n");
    E_INFO("frame_size %d frame_shift %d\n", frame_size, frame_shift);
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[0], &nfr) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, nfr);
    TEST_EQUAL(nfr, 1);
    TEST_EQUAL(inptr - buf, frame_size + frame_shift);

    nfr = 1;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[1], &nfr) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, nfr);
    TEST_EQUAL(nfr, 1);
    TEST_EQUAL(inptr - buf, frame_size + 2 * frame_shift);
    
    nfr = 1;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[2], &nfr) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, nfr);
    TEST_EQUAL(nfr, 1);
    TEST_EQUAL(inptr - buf, frame_size + 3 * frame_shift);

    /* Should consume all the input at this point. */
    nfr = 1;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[3], &nfr) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, nfr);
    TEST_EQUAL(nfr, 1);
    TEST_EQUAL(inptr - buf, 1024);
    TEST_EQUAL(nsamp, 0);

    /* Should get a frame here due to overflow samples. */
    TEST_ASSERT(fe_end_utt(fe, cepbuf1[4], &nfr) >= 0);
    E_INFO("fe_end_utt nfr %d\n", nfr);
    TEST_EQUAL(nfr, 1);

    /* Test that the output we get by processing one frame at a time
     * is exactly the same as what we get from doing them all at once. */
    E_INFO("Testing all data at once (1024 samples)\n");
    cepbuf2 = ckd_calloc_2d(5, DEFAULT_NUM_CEPSTRA, sizeof(**cepbuf2));
    inptr = &buf[0];
    nfr = 5;
    nsamp = 1024;
    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cepbuf2, &nfr) >= 0);
    E_INFO("fe_process_frames consumed nfr %d frames\n", nfr);
    TEST_EQUAL(nfr, 4);
    TEST_EQUAL(inptr - buf, 1024);
    TEST_EQUAL(nsamp, 0);
    /* And again, should get a frame here due to overflow samples. */
    TEST_ASSERT(fe_end_utt(fe, cepbuf2[4], &nfr) >= 0);
    E_INFO("fe_end_utt nfr %d\n", nfr);
    TEST_EQUAL(nfr, 1);

    /* output features stored in cepbuf should be the same */
    for (nfr = 0; nfr < 5; ++nfr) {
      E_INFO("%d: ", nfr);
      for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        E_INFOCONT("%.2f,%.2f ",
		   MFCC2FLOAT(cepbuf1[nfr][i]),
		   MFCC2FLOAT(cepbuf2[nfr][i]));
        TEST_EQUAL_MFCC(cepbuf1[nfr][i], cepbuf2[nfr][i]);
      }
      E_INFOCONT("\n");
    }
    
    /* Now, also test to make sure that even if we feed data in
     * little tiny bits we can still make things work. */
    E_INFO("Testing inputs smaller than one frame (256 samples)\n");
    memset(cepbuf2[0], 0, 5 * DEFAULT_NUM_CEPSTRA * sizeof(**cepbuf2));
    inptr = &buf[0];
    cptr = &cepbuf2[0];
    nfr = 5;
    i = 5;
    nsamp = 256;
    TEST_EQUAL(0, fe_start_utt(fe));
    /* Process up to 5 frames (that will not happen) */
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    /* Process up to however many frames are left to make 5 */
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    E_INFO("updated inptr %ld remaining nsamp %ld processed nfr %d\n",
	   inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    E_INFO("nfr %d\n", nfr);
    /* We processed 1024 bytes, which should give us 4 frames */
    TEST_EQUAL(nfr, 1);
    TEST_ASSERT(fe_end_utt(fe, *cptr, &nfr) >= 0);
    E_INFO("nfr %d\n", nfr);
    TEST_EQUAL(nfr, 1);

    /* output features stored in cepbuf should be the same */
    for (nfr = 0; nfr < 5; ++nfr) {
      E_INFO("%d: ", nfr);
      for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        E_INFOCONT("%.2f,%.2f ",
		   MFCC2FLOAT(cepbuf1[nfr][i]),
		   MFCC2FLOAT(cepbuf2[nfr][i]));
        TEST_EQUAL_MFCC(cepbuf1[nfr][i], cepbuf2[nfr][i]);
      }
      E_INFOCONT("\n");
    }

    /* And now, finally, test fe_process_utt() */
    E_INFO("Test fe_process_utt (apparently, it is deprecated)\n");
    inptr = &buf[0];
    i = 0;
    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    E_INFO("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    E_INFO("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    E_INFO("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    E_INFO("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_end_utt(fe, cepbuf2[i], &nfr) >= 0);
    E_INFO("i %d nfr %d\n", i, nfr);
    TEST_EQUAL(nfr, 1);

    /* output features stored in cepbuf should be the same */
    for (nfr = 0; nfr < 5; ++nfr) {
      E_INFO("%d: ", nfr);
      for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        E_INFOCONT("%.2f,%.2f ",
		   MFCC2FLOAT(cepbuf1[nfr][i]),
		   MFCC2FLOAT(cepbuf2[nfr][i]));
        TEST_EQUAL_MFCC(cepbuf1[nfr][i], cepbuf2[nfr][i]);
      }
      E_INFOCONT("\n");
    }

    /* Now test -remove_noise */
    fe_free(fe);
    ps_config_set_bool(config, "remove_noise", TRUE);
    TEST_ASSERT(fe = fe_init_auto_r(config));
    E_INFO("Testing all data at once (1024 samples)\n");
    inptr = &buf[0];
    nfr = 5;
    nsamp = 1024;
    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cepbuf2, &nfr) >= 0);
    E_INFO("fe_process_frames consumed nfr %d frames\n", nfr);
    TEST_EQUAL(nfr, 4);
    TEST_EQUAL(inptr - buf, 1024);
    TEST_EQUAL(nsamp, 0);
    /* And again, should get a frame here due to overflow samples. */
    TEST_ASSERT(fe_end_utt(fe, cepbuf2[4], &nfr) >= 0);
    E_INFO("fe_end_utt nfr %d\n", nfr);
    TEST_EQUAL(nfr, 1);

    /* output features stored in cepbuf will not be the same */
    E_INFO("Expect differences due to -remove_noise\n");
    for (nfr = 0; nfr < 5; ++nfr) {
      E_INFO("%d: ", nfr);
      for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        E_INFOCONT("%.2f,%.2f ",
		   MFCC2FLOAT(cepbuf1[nfr][i]),
		   MFCC2FLOAT(cepbuf2[nfr][i]));
      }
      E_INFOCONT("\n");
    }

    fe_free(fe);
    ckd_free_2d(cepbuf1);
    ckd_free_2d(cepbuf2);
    fclose(raw);
    ps_config_free(config);


    return 0;
}
