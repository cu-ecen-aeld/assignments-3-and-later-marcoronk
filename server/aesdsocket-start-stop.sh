#!/bin/sh

start () {
   echo 'Starting service aesdsocket'
   start-stop-daemon -S -n aesdsocket --exec /usr/bin/aesdsocket -- -d
}

stop () {
   echo 'Stopping service aesdsocket'
   start-stop-daemon --stop --oknodo --name aesdsocket --retry 5   
}

servicerestart () {
  stop
  sleep 5
  start
}

status() {
case "$(pidof aesdsocket | wc -w)" in

0)  echo "Service stopped."
    ;;
1)  echo "Service already running"
    ;;
esac

}

case "$1" in

  start)
    start
    ;;

  stop)
    stop
    ;;

  restart)    
    servicerestart
    ;;

  status)  
    status
    ;;    

  *)
    echo "Usage: $0 {start|stop|restart|status}"
    ;;
esac