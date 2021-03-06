#!/bin/bash

### BEGIN INIT INFO
# Provides:          @MODNAME@
# Required-Start:    $local_fs $syslog @RequiredStart@
# Required-Stop:     $local_fs $syslog @RequiredStop@
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Load kernel module and create device nodes at boot time.
# Description:       Load kernel module and create device nodes at boot time for @MODNAME@ instruments.
### END INIT INFO

case "$1" in
    start)
        /sbin/modprobe @MODNAME@
        exit 0
    ;;

    stop)
	/sbin/rmmod @MODNAME@
        exit 0
    ;;

    restart)
        /sbin/rmmod @MODNAME@
        /sbin/modprobe @MODNAME@
        exit 0
    ;;

    *)
        echo "Error: argument '$1' not supported." >&2
        exit 0
    ;;
esac


