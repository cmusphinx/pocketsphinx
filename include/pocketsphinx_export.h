#ifndef __POCKETSPHINX_EXPORT_H__
#define __POCKETSPHINX_EXPORT_H__

/* Win32/WinCE DLL gunk */
#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(CYGWIN)
#ifdef POCKETSPHINX_EXPORTS
#define POCKETSPHINX_EXPORT __declspec(dllexport)
#else
#define POCKETSPHINX_EXPORT __declspec(dllimport)
#endif
#else /* !_WIN32 */
#define POCKETSPHINX_EXPORT
#endif

#endif /* __POCKETSPHINX_EXPORT_H__ */
