#!/bin/sh

# Quick start-stop-daemon example, derived from Debian /etc/init.d/ssh
set -e

# Must be a valid filename
NAME=xash3d
PIDFILE=/var/run/$NAME.pid

#This is the command to be run, give the full pathname
DAEMON=/home/server/DedicatedServer/xash3d
DAEMON_OPTS="+map bounce -dev 3 -log"
UID=root

export PATH="${PATH:+$PATH:}/usr/sbin:/sbin"
export LD_LIBRARY_PATH="/home/server/DedicatedServer"

case "$1" in
  start)
        echo -n "Starting daemon: "$NAME
        start-stop-daemon --start --quiet --pidfile $PIDFILE --make-pidfile --exec $DAEMON --chuid $UID --background -- $DAEMON_OPTS
        echo "."
        ;;
  stop)
        echo -n "Stopping daemon: "$NAME
        start-stop-daemon --stop --quiet --oknodo --pidfile $PIDFILE --chuid $UID
        echo "."
        ;;
  restart)
        echo -n "Restarting daemon: "$NAME
        start-stop-daemon --stop --quiet --oknodo --retry 30 --pidfile $PIDFILE --chuid $UID
        start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON --chuid $UID --background --make-pidfile -- $DAEMON_OPTS
        echo "."
        ;;

  *)
        echo "Usage: "$1" {start|stop|restart}"
        exit 1
esac

exit 0
