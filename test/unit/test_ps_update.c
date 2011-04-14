#include <pocketsphinx.h>

static void test_reinit_fsg()
{
       ps_decoder_t *ps;
       cmd_ln_t *config;
       config = cmd_ln_init(NULL, ps_args(), TRUE,
			    "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
			    "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
			    "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                            NULL);
       ps = ps_init(config);
       ps_update_fsgset (ps);
       ps_update_fsgset (ps);
}

static void test_reinit_lm()
{
        ps_decoder_t *ps;
        cmd_ln_t *config;
        ngram_model_t *model;
        config = cmd_ln_init(NULL, ps_args(), TRUE,
    			     "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
			     "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
			     "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                             NULL);
        ps = ps_init(config);
        model = ps_update_lmset (ps, NULL);
}

int main(int argc, char argv[]) 
{
	test_reinit_fsg();
	test_reinit_lm();
	return 0;
}
