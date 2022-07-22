add_library(pocketsphinx
  tmat.c
  fsg_history.c
  allphone_search.c
  s2_semi_mgau.c
  ps_alignment.c
  ms_gauden.c
  state_align_search.c
  mdef.c
  blkarray_list.c
  dict2pid.c
  vector.c
  ms_senone.c
  ptm_mgau.c
  ngram_search_fwdflat.c
  kws_detections.c
  hmm.c
  bin_mdef.c
  phone_loop_search.c
  ngram_search.c
  ngram_search_fwdtree.c
  ms_mgau.c
  fsg_search.c
  ps_lattice.c
  fsg_lextree.c
  ps_mllr.c
  pocketsphinx.c
  kws_search.c
  acmod.c
  dict.c
  fe/fe_sigproc.c
  fe/fixlog.c
  fe/fe_warp_inverse_linear.c
  fe/fe_warp.c
  fe/fe_interface.c
  fe/fe_noise.c
  fe/fe_warp_affine.c
  fe/fe_warp_piecewise_linear.c
  fe/yin.c
  feat/cmn.c
  feat/agc.c
  feat/cmn_live.c
  feat/feat.c
  feat/lda.c
  lm/lm_trie_quant.c
  lm/ngram_model_trie.c
  lm/fsg_model.c
  lm/jsgf.c
  lm/ngram_model_set.c
  lm/ngrams_raw.c
  lm/jsgf_scanner.c
  lm/ngram_model.c
  lm/lm_trie.c
  lm/jsgf_parser.c
  util/strfuncs.c
  util/dtoa.c
  util/case.c
  util/filename.c
  util/slamch.c
  util/cmd_ln.c
  util/blas_lite.c
  util/mmio.c
  util/hash_table.c
  util/err.c
  util/ckd_alloc.c
  util/slapack_lite.c
  util/matrix.c
  util/sbthread.c
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
  util/bitarr.c
  util/pio.c
  util/genrand.c
  )
target_include_directories(
  pocketsphinx PRIVATE ${CMAKE_BINARY_DIR} # config.h
  pocketsphinx PRIVATE ${CMAKE_SOURCE_DIR}/include/pocketsphinx
  pocketsphinx PUBLIC ${CMAKE_SOURCE_DIR}/include
  pocketsphinx PUBLIC ${CMAKE_BINARY_DIR}/include
  pocketsphinx INTERFACE ${CMAKE_SOURCE_DIR}/include
  )
if(APPLE)
  # Things we might need are here
  target_link_directories(pocketsphinx PUBLIC /usr/local/lib)
endif()
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(pocketsphinx PUBLIC ${MATH_LIBRARY})
endif()
find_package(Threads)
target_link_libraries(pocketsphinx PUBLIC Threads::Threads)
# Shared library version != package version, but we will make it the
# same for now to avoid confusion
set_target_properties(pocketsphinx PROPERTIES
  VERSION 5.0.0
  SOVERSION 5
  )
# No idea why this can't just go in the above list but oh well
set_property(TARGET pocketsphinx PROPERTY
  COMPILE_DEFINITIONS POCKETSPHINX_EXPORTS;SPHINXBASE_EXPORTS
)
