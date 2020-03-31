/* SphinxBase headesr. */
#include <sphinxbase/fe.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/mmio.h>

/* Local headers. */
#include "acmod.h"
#include "hmm.h"
#include "bin_mdef.h"
#include "ms_gauden.h"

typedef struct nn_mgau_s nn_mgau_t;
struct nn_mgau_s {
    ps_mgau_t base;
    int keras_server_sock;
    int n_feats;
};

ps_mgau_t *nn_mgau_init(acmod_t *acmod, bin_mdef_t *mdef);
void nn_mgau_free(ps_mgau_t *s);
int nn_mgau_frame_eval(ps_mgau_t *s,
                        int16 *senone_scores,
                        uint8 *senone_active,
                        int32 n_senone_active,
                        mfcc_t **featbuf,
                        int32 frame,
                        int32 compallsen);
int nn_mgau_mllr_transform(ps_mgau_t *s,
                            ps_mllr_t *mllr);