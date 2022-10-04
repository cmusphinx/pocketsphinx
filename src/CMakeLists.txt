add_library(pocketsphinx
acmod.c
allphone_search.c
bin_mdef.c
common_audio/vad/vad_gmm.c
common_audio/vad/webrtc_vad.c
common_audio/vad/vad_filterbank.c
common_audio/vad/vad_core.c
common_audio/vad/vad_sp.c
common_audio/signal_processing/division_operations.c
common_audio/signal_processing/resample_48khz.c
common_audio/signal_processing/resample.c
common_audio/signal_processing/resample_fractional.c
common_audio/signal_processing/downsample_fast.c
common_audio/signal_processing/min_max_operations.c
common_audio/signal_processing/cross_correlation.c
common_audio/signal_processing/vector_scaling_operations.c
common_audio/signal_processing/resample_by_2_internal.c
common_audio/signal_processing/energy.c
common_audio/signal_processing/spl_inl.c
common_audio/signal_processing/get_scaling_square.c
dict2pid.c
dict.c
fe/fe_sigproc.c
fe/fixlog.c
fe/fe_warp_inverse_linear.c
fe/fe_noise.c
fe/fe_warp.c
fe/fe_interface.c
fe/fe_warp_affine.c
fe/yin.c
fe/fe_warp_piecewise_linear.c
feat/cmn.c
feat/agc.c
feat/cmn_live.c
feat/feat.c
feat/lda.c
fsg_history.c
fsg_lextree.c
fsg_search.c
hmm.c
kws_detections.c
kws_search.c
lm/lm_trie_quant.c
lm/ngram_model_trie.c
lm/fsg_model.c
lm/jsgf.c
lm/ngram_model_set.c
lm/ngrams_raw.c
lm/jsgf_scanner.c
lm/bitarr.c
lm/ngram_model.c
lm/lm_trie.c
lm/jsgf_parser.c
mdef.c
ms_gauden.c
ms_mgau.c
ms_senone.c
ngram_search.c
ngram_search_fwdflat.c
ngram_search_fwdtree.c
phone_loop_search.c
pocketsphinx.c
ps_alignment.c
ps_config.c
ps_endpointer.c
ps_lattice.c
ps_mllr.c
ps_vad.c
ptm_mgau.c
s2_semi_mgau.c
state_align_search.c
tmat.c
util/strfuncs.c
util/dtoa.c
util/case.c
util/filename.c
util/slamch.c
util/cmd_ln.c
util/blas_lite.c
util/blkarray_list.c
util/vector.c
util/mmio.c
util/hash_table.c
util/err.c
util/ckd_alloc.c
util/slapack_lite.c
util/matrix.c
util/bio.c
util/heap.c
util/priority_queue.c
util/bitvec.c
util/profile.c
util/errno.c
util/logmath.c
util/glist.c
util/f2c_lite.c
util/listelem_alloc.c
util/pio.c
util/genrand.c
util/soundfiles.c
  )
target_include_directories(
  pocketsphinx PRIVATE ${CMAKE_BINARY_DIR}
  pocketsphinx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  pocketsphinx PUBLIC ${CMAKE_SOURCE_DIR}/include
  pocketsphinx PUBLIC ${CMAKE_BINARY_DIR}/include
  pocketsphinx INTERFACE ${CMAKE_SOURCE_DIR}/include
  pocketsphinx INTERFACE ${CMAKE_BINARY_DIR}/include
  )
if(APPLE)
  # Things we might need are here
  target_link_directories(pocketsphinx PUBLIC /usr/local/lib)
endif()
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(pocketsphinx PUBLIC ${MATH_LIBRARY})
endif()
# Shared library version != package version, but we will make it the
# same for now to avoid confusion
set_target_properties(pocketsphinx PROPERTIES
  VERSION 5.0.0
  SOVERSION 5
  )
# No idea why this can't just go in the above list but oh well
if(MODELDIR)
  set_property(TARGET pocketsphinx PROPERTY
    COMPILE_DEFINITIONS POCKETSPHINX_EXPORTS;SPHINXBASE_EXPORTS;MODELDIR="${MODELDIR}"
    )
else()
  set_property(TARGET pocketsphinx PROPERTY
    COMPILE_DEFINITIONS POCKETSPHINX_EXPORTS;SPHINXBASE_EXPORTS)
endif()
