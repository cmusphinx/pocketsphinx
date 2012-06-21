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
       ps_free(ps);
       cmd_ln_free_r(config);
}

static void test_reinit_fsg_missing()
{
       ps_decoder_t *ps;
       cmd_ln_t *config;
       config = cmd_ln_init(NULL, ps_args(), TRUE,
			    "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
			    "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                            NULL);
       ps = ps_init(config);
       cmd_ln_set_str_r(config, "-fsg", "/some/fsg");
       ps_update_fsgset (ps);
       ps_free(ps);
       cmd_ln_free_r(config);
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
        ps_free(ps);
        cmd_ln_free_r(config);
}

int main(int argc, char* argv[]) 
{
	test_reinit_fsg();
	test_reinit_fsg_missing();
	test_reinit_lm();
	return 0;
}
