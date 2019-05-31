dnl
dnl
dnl This will check if ipv6 is available
dnl
dnl

AC_DEFUN([AC_DVTS_CHECK_INET6], [
AC_MSG_CHECKING([if ipv6 is available])
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
int main(void)
{
  if (socket(AF_INET6, SOCK_STREAM, 0) < 0) {
    return(1);
  }
  else {
    return(0);
  }
}
  ],
  [
    AC_DEFINE(ENABLE_INET6)
    ipv6=yes
  ],
  [
    ipv6=no
  ],
  [
    ipv6=no
  ])
AC_MSG_RESULT($ipv6)

ipv6type=unknown
ipv6lib=none
ipv6trylibc=yes

if test "$ipv6" = "yes"; then
  AC_MSG_CHECKING([ipv6 stack type])
    for i in inria kame linux-glibc linux-inet6 toshiba v6d zeta solaris; do
      case $i in
        inria)
        # http://www.kame.net/
        AC_EGREP_CPP(yes, [
#include <netinet/in.h>
#ifdef IPV6_INRIA_VERSION
yes
#endif],
          [
            ipv6type=$i
            CFLAGS="-DINET6 $CFLAGS"
          ])
        ;;

      kame)
        # http://www.kame.net/
        AC_EGREP_CPP(yes, [
#include <netinet/in.h>
#ifdef __KAME__
yes
#endif],
          [
            ipv6type=$i
            ipv6lib=inet6
            ipv6libdir=/usr/local/v6/lib
            ipv6trylibc=yes
            CFLAGS="-DINET6 $CFLAGS"
          ])
        ;;

      linux-glibc)
        # http://www.v6.linux.or.jp/
        AC_EGREP_CPP(yes, [
#include <features.h>
#if defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
yes
#endif],
          [
            ipv6type=$i;
            CFLAGS="-DINET6 $CFLAGS"
          ])
        ;;

      linux-inet6)
        # http://www.v6.linux.or.jp/
        if test -d /usr/inet6; then
          ipv6type=$i
          ipv6lib=inet6
          ipv6libdir=/usr/inet6/lib
          CFLAGS="-DINET6 -I/usr/inet6/include $CFLAGS"
        fi
        ;;

      toshiba)
        AC_EGREP_CPP(yes, [
#include <sys/param.h>
#ifdef _TOSHIBA_INET6
yes
#endif],
          [
            ipv6type=$i
            ipv6lib=inet6
            ipv6libdir=/usr/local/v6/lib
            CFLAGS="-DINET6 $CFLAGS"
          ])
        ;;

      v6d)
        AC_EGREP_CPP(yes, [
#include </usr/local/v6/include/sys/v6config.h>
#ifdef __V6D__
yes
#endif],
          [
            ipv6type=$i
            ipv6lib=v6
            ipv6libdir=/usr/local/v6/lib
            CFLAGS="-I/usr/local/v6/include $CFLAGS"
          ])
        ;;

      zeta)
        AC_EGREP_CPP(yes, [
#include <sys/param.h>
#ifdef _ZETA_MINAMI_INET6
yes
#endif],
          [
            ipv6type=$i
            ipv6lib=inet6
            ipv6libdir=/usr/local/v6/lib
             CFLAGS="-DINET6 $CFLAGS"
          ])
        ;;

      solaris)
        AC_EGREP_CPP(yes, [
#include <sys/elf_notes.h>
#ifdef ELF_NOTE_SOLARIS
yes
#endif],
          [
            ipv6type=$i
          ])
        ;;

      esac
    if test "$ipv6type" != "unknown"; then
      break
    fi
  done
  AC_MSG_RESULT($ipv6type)
fi

if test "$ipv6" = "yes" -a "$ipv6lib" != "none"; then
  if test -d $ipv6libdir -a -f $ipv6libdir/lib$ipv6lib.a; then
    LIBS="-L$ipv6libdir -l$ipv6lib $LIBS"
    echo "You have $ipv6lib library, using it"
  else
    if test "$ipv6trylibc" = "yes"; then
      echo "You do not have $ipv6lib library, using libc"
    else
      echo 'Fatal: no $ipv6lib library found.  cannot continue.'
      echo "You need to fetch lib$ipv6lib.a from appropriate"
      echo 'ipv6 kit and compile beforehand.'
      exit 1
    fi
  fi
fi
])
])
