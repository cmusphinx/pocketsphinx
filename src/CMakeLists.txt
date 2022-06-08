add_library(pocketsphinx
  libpocketsphinx/tmat.c
  libpocketsphinx/fsg_history.c
  libpocketsphinx/allphone_search.c
  libpocketsphinx/s2_semi_mgau.c
  libpocketsphinx/ps_alignment.c
  libpocketsphinx/ms_gauden.c
  libpocketsphinx/state_align_search.c
  libpocketsphinx/mdef.c
  libpocketsphinx/blkarray_list.c
  libpocketsphinx/dict2pid.c
  libpocketsphinx/vector.c
  libpocketsphinx/ms_senone.c
  libpocketsphinx/ptm_mgau.c
  libpocketsphinx/ngram_search_fwdflat.c
  libpocketsphinx/kws_detections.c
  libpocketsphinx/hmm.c
  libpocketsphinx/bin_mdef.c
  libpocketsphinx/phone_loop_search.c
  libpocketsphinx/ngram_search.c
  libpocketsphinx/ngram_search_fwdtree.c
  libpocketsphinx/ms_mgau.c
  libpocketsphinx/fsg_search.c
  libpocketsphinx/ps_lattice.c
  libpocketsphinx/fsg_lextree.c
  libpocketsphinx/ps_mllr.c
  libpocketsphinx/pocketsphinx.c
  libpocketsphinx/kws_search.c
  libpocketsphinx/acmod.c
  libpocketsphinx/dict.c
  libsphinxbase/fe/fe_sigproc.c
  libsphinxbase/fe/fixlog.c
  libsphinxbase/fe/fe_prespch_buf.c
  libsphinxbase/fe/fe_warp_inverse_linear.c
  libsphinxbase/fe/fe_noise.c
  libsphinxbase/fe/fe_warp.c
  libsphinxbase/fe/fe_interface.c
  libsphinxbase/fe/fe_warp_affine.c
  libsphinxbase/fe/yin.c
  libsphinxbase/fe/fe_warp_piecewise_linear.c
  libsphinxbase/feat/cmn.c
  libsphinxbase/feat/agc.c
  libsphinxbase/feat/cmn_live.c
  libsphinxbase/feat/feat.c
  libsphinxbase/feat/lda.c
  libsphinxbase/lm/lm_trie_quant.c
  libsphinxbase/lm/ngram_model_trie.c
  libsphinxbase/lm/fsg_model.c
  libsphinxbase/lm/jsgf.c
  libsphinxbase/lm/ngram_model_set.c
  libsphinxbase/lm/ngrams_raw.c
  libsphinxbase/lm/jsgf_scanner.c
  libsphinxbase/lm/ngram_model.c
  libsphinxbase/lm/lm_trie.c
  libsphinxbase/lm/jsgf_parser.c
  libsphinxbase/util/strfuncs.c
  libsphinxbase/util/dtoa.c
  libsphinxbase/util/case.c
  libsphinxbase/util/filename.c
  libsphinxbase/util/slamch.c
  libsphinxbase/util/cmd_ln.c
  libsphinxbase/util/blas_lite.c
  libsphinxbase/util/mmio.c
  libsphinxbase/util/hash_table.c
  libsphinxbase/util/err.c
  libsphinxbase/util/ckd_alloc.c
  libsphinxbase/util/slapack_lite.c
  libsphinxbase/util/matrix.c
  libsphinxbase/util/sbthread.c
  libsphinxbase/util/bio.c
  libsphinxbase/util/heap.c
  libsphinxbase/util/priority_queue.c
  libsphinxbase/util/bitvec.c
  libsphinxbase/util/profile.c
  libsphinxbase/util/errno.c
  libsphinxbase/util/logmath.c
  libsphinxbase/util/glist.c
  libsphinxbase/util/f2c_lite.c
  libsphinxbase/util/listelem_alloc.c
  libsphinxbase/util/bitarr.c
  libsphinxbase/util/pio.c
  libsphinxbase/util/genrand.c
  )
target_include_directories(
  pocketsphinx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libpocketsphinx
  pocketsphinx PRIVATE ${CMAKE_BINARY_DIR} # config.h
  pocketsphinx PRIVATE ${CMAKE_SOURCE_DIR}/include/pocketsphinx
  pocketsphinx PUBLIC ${CMAKE_SOURCE_DIR}/include
  pocketsphinx PUBLIC ${CMAKE_BINARY_DIR}/include
  pocketsphinx INTERFACE ${CMAKE_SOURCE_DIR}/include
  )
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(pocketsphinx PUBLIC ${MATH_LIBRARY})
endif()
find_package(Threads)
target_link_libraries(pocketsphinx PUBLIC Threads::Threads)
set_target_properties(pocketsphinx PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})

add_subdirectory(programs)
