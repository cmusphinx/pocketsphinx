#ifndef __SPHINXBASE_EXPORT_H__
#define __SPHINXBASE_EXPORT_H__

/* Win32 DLL gunk */
#if defined(_WIN32) && defined(SPHINX_DLL)
#if defined(SPHINXBASE_EXPORTS) /* DLL itself */
#define SPHINXBASE_EXPORT __declspec(dllexport)
#else
#define SPHINXBASE_EXPORT __declspec(dllimport)
#endif
#else /* No DLL things*/
#define SPHINXBASE_EXPORT
#endif

#endif /* __SPHINXBASE_EXPORT_H__ */
