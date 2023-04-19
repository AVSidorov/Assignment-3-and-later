#!/bin/sh
#


# Variables
DAEMON="aesdsocket"
DAEMON_PATH="/usr/bin/aesdsocket"
#DAEMON_PATH=$(dirname $(realpath $0))/$DAEMON
DAEMONOPTS="-d"

NAME="aesdsocket"
DESC="Simple socket server"
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/S99$NAME


d_start() {
        echo "Starting $DESC"
        start-stop-daemon -S -b -n $NAME -a $DAEMON_PATH -- $DAEMONOPTS
}

d_stop() {
        echo "Stopping $DESC"
        start-stop-daemon --stop -n $NAME
}

case "$1" in
        start)
                d_start
                ;;
        stop)
                d_stop
                ;;
        restart)
                d_stop
                d_start
                ;;
        *)
                echo "Usage: $SCRIPTNAME {start|stop|restart}" >&2
                exit 3
                ;;
esac

exit 0
