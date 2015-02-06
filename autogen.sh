#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
PKG_NAME="the package."

DIE=0

# Check tools

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

(grep "^LT_INIT" $srcdir/configure.ac >/dev/null) && {
  if libtoolize --version </dev/null >/dev/null 2>&1; then
	LIBTOOLIZE=libtoolize
  elif glibtoolize --version </dev/null >/dev/null 2>&1; then
	LIBTOOLIZE=glibtoolize
  else
    echo
    echo "**Error**: You must have \`libtool' installed."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool/libtool-2.2.6b.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  fi 
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.11.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.11.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find $srcdir -name configure.ac -print`
do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    macrodirs=`sed -n -e 's,AM_ACLOCAL_INCLUDE(\(.*\)),\1,gp' < $coin`
    ( cd $dr
      aclocalinclude="$ACLOCAL_FLAGS"
      for k in $macrodirs; do
  	if test -d $k; then
          aclocalinclude="$aclocalinclude -I $k"
  	##else 
	##  echo "**Warning**: No such directory \`$k'.  Ignored."
        fi
      done
      if grep "^LT_INIT" configure.ac >/dev/null; then
	echo "Running $LIBTOOLIZE..."
	$LIBTOOLIZE --force --copy
      fi
      echo "Running aclocal $aclocalinclude ..."
      aclocal -I m4 $aclocalinclude
      if grep "^AC_CONFIG_HEADER" configure.ac >/dev/null; then
	echo "Running autoheader..."
	autoheader
      fi
      echo "Running automake --foreign --copy $am_opt ..."
      automake --add-missing --foreign --copy $am_opt
      echo "Running autoconf ..."
      autoconf
    )
  fi
done

#conf_flags="--enable-maintainer-mode --enable-compile-warnings" #--enable-iso-c

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME
else
  echo Skipping configure process.
fi
