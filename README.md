led_wall
========

Led Wall

contents of bash_profile:
========

    sleep 2
    
    while :
    do
    	echo "starting capture"
    	gst-launch --gst-plugin-path=/home/wall/led_wall/gst/ v4l2src ! 'video/x-raw-yuv,format=(fourcc)YUY2;video/x-raw-rgb' ! ledwallvideosink
    	sleep 1
    done

mingetty setup /etc/init/tty1.conf
====

    # tty1 - getty
    #
    # This service maintains a getty on tty1 from the point the system is
    # started until it is shut down again.
    
    start on stopped rc RUNLEVEL=[2345] and (
                not-container or
                container CONTAINER=lxc or
                container CONTAINER=lxc-libvirt)
    
    stop on runlevel [!2345]
    
    respawn
    #exec /sbin/getty -8 38400 tty1
    exec /sbin/mingetty --autologin wall tty1

thanks
====

http://riderx.info/post/The-LPD8806-protocol-for-Adafruit-RGB-LED-Strips.aspx [helped me understand the protocol]
https://www.pjrc.com/ [wrote the microcontroller code.. USB -> 8 parallel data/clock serial pairs]
