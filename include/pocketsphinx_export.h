#ifndef __POCKETSPHINX_EXPORT_H__
#define __POCKETSPHINX_EXPORT_H__

/* Win32 DLL gunk */
#if defined(_WIN32) && defined(SPHINX_DLL)
#if defined(POCKETSPHINX_EXPORTS) /* DLL itself */
#define POCKETSPHINX_EXPORT __declspec(dllexport)
#else
#define POCKETSPHINX_EXPORT __declspec(dllimport)
#endif
#else /* No DLL things*/
#define POCKETSPHINX_EXPORT
#endif

#endif /* __POCKETSPHINX_EXPORT_H__ */
