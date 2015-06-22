PocketSphinx 5prealpha
===============================================================================

This is PocketSphinx, one of Carnegie Mellon University's open source large
vocabulary, speaker-independent continuous speech recognition engine.

**THIS IS A RESEARCH SYSTEM**. This is also an early release of a research
system. We know the APIs and function names are likely to change, and that
several tools need to be made available to make this all complete. With your
help and contributions, this can progress in response to the needs and patches
provided.

**Please see the LICENSE file for terms of use.**

Prerequisites
-------------------------------------------------------------------------------

You **must** have SphinxBase, which you can download from
http://cmusphinx.sourceforge.net. Download and unpack it to the same parent
directory as PocketSphinx, so that the configure script and project files can
find it. On Windows, you will need to rename 'sphinxbase-X.Y' (where X.Y is the
SphinxBase version number) to simply 'sphinxbase' for this to work.

Linux/Unix installation
------------------------------------------------------------------------------

In a unix-like environment (such as linux, solaris etc):

 * Build and optionally install SphinxBase. If you want to use
   fixed-point arithmetic, you **must** configure SphinxBase with the
   `--enable-fixed` option.

 * If you downloaded directly from the CVS repository, you need to do
   this at least once to generate the "configure" file:

   ```
   $ ./autogen.sh
   ```
 * If you downloaded the release version, or ran `autogen.sh` at least
   once, then compile and install:

   ```
   $ ./configure
   $ make clean all
   $ make check
   $ sudo make install
   ```

XCode Installation (for iPhone)
------------------------------------------------------------------------------

Pocketsphinx uses the standard unix autogen system, you can build pocketsphinx
with automake given you already built sphinxbase You just need to pass correct
configure arguments, set compiler path, set sysroot and other options. After
you build the code you need to import dylib file into your project and you also
need to configure includes for your project to find sphinxbase headers.

You also will have to create a recorder to capture audio with CoreAudio and
feed it into the recognizer.

For details see http://github.com/cmusphinx/pocketsphinx-ios-demo

If you want to quickly start with Pocketsphinx, try OpenEars toolkit which
includes Pocketsphinx http://www.politepix.com/openears/


Android installation
------------------------------------------------------------------------------

See http://github.com/cmusphinx/pocketsphinx-android-demo.


MS Windows™ (MS Visual Studio 2010 (or newer - we test with VC++ 2010 Express)
------------------------------------------------------------------------------

 * load sphinxbase.sln located in sphinxbase directory

 * compile all the projects in SphinxBase (from sphinxbase.sln)

 * load pocketsphinx.sln in pocketsphinx directory

 * compile all the projects in PocketSphinx

MS Visual Studio will build the executables under .\bin\Release or
.\bin\Debug (depending on the version you choose on MS Visual Studio),
and the libraries under .\lib\Release or .\lib\Build.

Test scripts are forthcoming for Windows.

For up-to-date information, see http://cmusphinx.sourceforge.net.
