AC_DEFUN([AC_DVTS_CHECK_LINUX_SSFAMILY],
[
  linux_ssfamily=no
  AC_MSG_CHECKING([if struct sockaddr_storage has __ss_family])
  AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
int main(void)
{
  struct sockaddr_storage ss;
  ss.__ss_family = 0;
  return(0);
}
  ],
  [
    CFLAGS="-DLINUX_OLD_SS_FAMILY $CFLAGS"
    linux_ssfamily=yes
  ],
  [
    linux_ssfamily=no
  ],
  [
    linux_ssfamily=no
  ])

AC_MSG_RESULT($linux_ssfamily)

])
