/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/**
 * @file bogus_pygobject.h
 * @brief Bogus header file to avoid having to depend on PyGTK.
 */
#ifndef __BOGUS_PYGOBJECT_H__
#define __BOGUS_PYGOBJECT_H__

#include "Python.h"

typedef struct {
    PyObject_HEAD
    void * boxed;
    unsigned long gtype;
    int free_on_dealloc;
} PyGBoxed;

typedef struct {
    PyObject_HEAD
    void *pointer;
    unsigned long gtype;
} PyGPointer;

#endif /* __BOGUS_PYGOBJECT_H__ */
