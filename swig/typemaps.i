%include <exception.i>

// Support for return of raw data
#if SWIGJAVA
%include <arrays_java.i>
%apply short[] {const short *SDATA};

%typemap(in, numinputs=0, noblock=1) int *RAWDATA_SIZE {
   int temp_len;
   $1 = &temp_len;
}
%typemap(jstype) short *get_rawdata "short[]"
%typemap(jtype) short *get_rawdata "short[]"
%typemap(jni) short *get_rawdata "jshortArray"
%typemap(javaout) short *get_rawdata {
  return $jnicall;
}
%typemap(out) short *get_rawdata {
  $result = JCALL1(NewShortArray, jenv, temp_len);
  JCALL4(SetShortArrayRegion, jenv, $result, 0, temp_len, $1);
}

#elif SWIGCSHARP
%include <arrays_csharp.i>
%apply unsigned char INPUT[] {const unsigned char *SDATA};
#endif

// Define typemaps to wrap error codes returned by some functions,
// into runtime exceptions.
%typemap(in, numinputs=0, noblock=1) int *errcode {
  int errcode;
  $1 = &errcode;
}

%typemap(argout) int *errcode {
  if (*$1 < 0) {
    char buf[64];
    snprintf(buf, 64, "$symname returned %d", *$1);
    SWIG_exception(SWIG_RuntimeError, buf);
  }
}

// Typemap for string arrays used in ngram API
#if SWIGPYTHON

%typemap(in) (size_t n, char **ptr) {
  /* Check if is a list */
  $1 = 0;
  if (PyList_Check($input)) {
    int i;
    $1 = PyList_Size($input);
    $2 = (char **) calloc(($1 + 1), sizeof(char *));
    for (i = 0; i < $1; i++) {
      PyObject *o = PyList_GetItem($input,i);
      $2[i] = SWIG_Python_str_AsChar(o);
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "list type expected");
    return NULL;
  }
}

%typemap(freearg) (size_t n, char **ptr) {
  int i;
  if ($2 != NULL) {
    for (i = 0; $2[i] != NULL; i++) {
        SWIG_Python_str_DelForPy3($2[i]);
    }
    free($2);
  }
}

#elif SWIGJAVA
%typemap(in) (size_t n, char **ptr) {
  int i = 0;
  $1 = (*jenv)->GetArrayLength(jenv, $input);
  $2 = (char **) malloc(($1)*sizeof(char *));
  /* make a copy of each string */
  for (i = 0; i<$1; i++) {
    jstring j_string = (jstring)(*jenv)->GetObjectArrayElement(jenv, $input, i);
    const char * c_string = (*jenv)->GetStringUTFChars(jenv, j_string, 0);
    $2[i] = malloc((strlen(c_string)+1)*sizeof(char));
    strcpy($2[i], c_string);
    (*jenv)->ReleaseStringUTFChars(jenv, j_string, c_string);
    (*jenv)->DeleteLocalRef(jenv, j_string);
  }
}

%typemap(freearg) (size_t n, char **ptr) {
  int i;
  for (i=0; i<$1; i++)
    free($2[i]);
  free($2);
}

%typemap(jni) (size_t n, char **ptr) "jobjectArray"
%typemap(jtype) (size_t n, char **ptr) "String[]"
%typemap(jstype) (size_t n, char **ptr) "String[]"
%typemap(javain) (size_t n, char **ptr) "$javainput"

#endif
