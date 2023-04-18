#!/bin/bash
#!/bin/sh
#
# chkconfig: 345 99 01
# description: My program daemon
#
# processname: my_program
# pidfile: /var/run/my_program.pid

# Source function library.
#. /etc/rc.d/init.d/functions

# Variables
DAEMON="aesdsocket"
DAEMON_PATH="/usr/bin/aesdsocket"
#DAEMON_PATH=$(dirname $(realpath $0))/$DAEMON
DAEMONOPTS="-d"

NAME="aesdsocket"
DESC="Simple socket server"
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/S99$NAME

# Check if the daemon is there.
test -x $DAEMON || exit 0

d_start() {
        echo "Starting $DESC"
        start-stop-daemon --start --background -n $NAME -a $DAEMON_DAEMON_PATH ${DAEMON_PATH} -- ${DAEMONOPTS}
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
