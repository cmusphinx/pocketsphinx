# Build the native component of the PocketSphinx library for Android.

# You MUST change this to the absolute path of the directory containing
# sphinxbase and pocketsphinx source code.
SPHINX_PATH := $(HOME)/Projects/Sphinx/trunk

# Copy this Android.mk along with pocketsphinx_wrap.c and the contents of the 'edu' folder
# built by swig to the jni/ directory of your Android project.
BASE_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(SPHINX_PATH)/sphinxbase/include/android $(SPHINX_PATH)/sphinxbase/include
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_PATH := $(SPHINX_PATH)/sphinxbase/src/libsphinxbase/util
LOCAL_MODULE := sphinxutil

LOCAL_SRC_FILES := \
	bio.c \
	bitvec.c \
	case.c \
	ckd_alloc.c \
	cmd_ln.c \
	dtoa.c \
	err.c \
	errno.c \
	f2c_lite.c \
	filename.c \
	genrand.c \
	glist.c \
	hash_table.c \
	heap.c \
	huff_code.c \
	info.c \
	listelem_alloc.c \
	logmath.c.arm \
	matrix.c \
	mmio.c \
	pio.c \
	profile.c \
	sbthread.c \
	strfuncs.c \
	utf8.c

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(SPHINX_PATH)/sphinxbase/include/android $(SPHINX_PATH)/sphinxbase/include
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_PATH := $(SPHINX_PATH)/sphinxbase/src/libsphinxbase/fe
LOCAL_MODULE := sphinxfe
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	fe_interface.c \
	fe_sigproc.c \
	fe_warp_affine.c \
	fe_warp.c \
	fe_warp_inverse_linear.c \
	fe_warp_piecewise_linear.c \
	fixlog.c

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(SPHINX_PATH)/sphinxbase/include/android $(SPHINX_PATH)/sphinxbase/include
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_PATH := $(SPHINX_PATH)/sphinxbase/src/libsphinxbase/feat
LOCAL_MODULE := sphinxfeat

LOCAL_SRC_FILES := \
	agc.c \
	cmn.c \
	cmn_prior.c \
	feat.c \
	lda.c

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(SPHINX_PATH)/sphinxbase/include/android $(SPHINX_PATH)/sphinxbase/include
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_PATH := $(SPHINX_PATH)/sphinxbase/src/libsphinxbase/lm
LOCAL_MODULE := sphinxlm

LOCAL_SRC_FILES := \
	fsg_model.c \
	jsgf.c \
	jsgf_parser.c \
	jsgf_scanner.c \
	lm3g_model.c \
	ngram_model_arpa.c \
	ngram_model_dmp.c \
	ngram_model_set.c \
	ngram_model.c

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(SPHINX_PATH)/sphinxbase/include/android $(SPHINX_PATH)/sphinxbase/include \
					$(SPHINX_PATH)/pocketsphinx/include
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_PATH := $(SPHINX_PATH)/pocketsphinx/src/libpocketsphinx
LOCAL_MODULE := pocketsphinx

LOCAL_SRC_FILES := \
	acmod.c     \
	bin_mdef.c    \
	blkarray_list.c   \
	dict.c     \
	dict2pid.c    \
	fsg_history.c   \
	fsg_lextree.c   \
	fsg_search.c   \
	hmm.c.arm     \
	mdef.c     \
	ms_gauden.c.arm    \
	ms_mgau.c.arm    \
	ms_senone.c.arm    \
	ngram_search.c   \
	ngram_search_fwdtree.c \
	ngram_search_fwdflat.c \
	phone_loop_search.c  \
	pocketsphinx.c \
	ps_lattice.c   \
	ps_mllr.c    \
	ptm_mgau.c.arm    \
	s2_semi_mgau.c.arm   \
	tmat.c     \
	vector.c

include $(BUILD_STATIC_LIBRARY)

# Create the dynamic library wrapper
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(SPHINX_PATH)/sphinxbase/include/android $(SPHINX_PATH)/sphinxbase/include \
					$(SPHINX_PATH)/pocketsphinx/include
LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_PATH := $(BASE_PATH)
LOCAL_MODULE := pocketsphinx_jni

LOCAL_SRC_FILES := pocketsphinx_wrap.c

PRIVATE_WHOLE_STATIC_LIBRARIES := \
	$(call static-library-path,sphinxutil) \
	$(call static-library-path,sphinxfe) \
	$(call static-library-path,sphinxfeat) \
	$(call static-library-path,sphinxlm) \
	$(call static-library-path,pocketsphinx)
LOCAL_STATIC_LIBRARIES := sphinxutil sphinxfe sphinxfeat sphinxlm pocketsphinx

include $(BUILD_SHARED_LIBRARY)
