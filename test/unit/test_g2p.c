#include <string.h>

#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/ngram_model.h>

#include "dict.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    logmath_t *logmath;
    ngram_model_t *model;
    FILE *fp;
    char *line = NULL, *grapheme, *phoneme, *predicted_phoneme;
    int32 different_word_count = 0, fit_count = 0;
    size_t len = 0;

    err_set_logfp(NULL);
    logmath = logmath_init(1.0001f, 0, 0);
    model = ngram_model_read(NULL, "../../test/data/g2p/train.dmp", NGRAM_AUTO, logmath);

    fp = fopen("../../test/data/g2p/test.dict", "r");

    while (getline(&line, &len, fp) != -1) {
        grapheme = strtok(line, "\t");
        phoneme = strtok(NULL, "\n");

        if (strchr(grapheme, '(')) {
            grapheme = strtok(grapheme, "(");
        } else {
            different_word_count++;
        }

        predicted_phoneme = dict_g2p(model, grapheme, G2P_SEARCH_WIDTH);
        if (predicted_phoneme && strcmp(phoneme, predicted_phoneme) == 0) {
            fit_count++;
        }
        ckd_free(predicted_phoneme);

    }

    TEST_ASSERT(fit_count * 1.0 / different_word_count > 0.6);

    ckd_free(grapheme);
    fclose(fp);
    ngram_model_free(model);
    logmath_free(logmath);
}
