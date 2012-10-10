LED WALL!!!
========

Built by Hans Lindauer, programmed by Alex Norman for Starfucker

build details:

http://dorkbotpdx.org/blog/armatronix/led_video_wall

contents of bash_profile:
========

    sleep 2
    
    while :
    do
    	echo "starting capture"
        /home/wall/led_wall/run_wall
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

ubuntu dependencies, I think
=====
    libgstreamer-plugins-base0.10-dev libgstreamer0.10-dev build-essential pkg-conf gstreamer0.10-tools gstreamer0.10-plugins-good gstreamer0.10-plugins-bad gstreamer0.10-plugins-ugly

thanks
====

* http://riderx.info/post/The-LPD8806-protocol-for-Adafruit-RGB-LED-Strips.aspx [helped me understand the protocol]
* https://www.pjrc.com/ [wrote the microcontroller code.. USB -> 8 parallel data/clock serial pairs]
* folks on the gstreamer freenode irc channel
* August Black, Wes Smith, various video advice
* http://www.polyvinylrecords.com/artists/index.php?id=824 STRFKR folks for providing us with this opportunity
* http://stackoverflow.com/users/198536/wallyk [answered http://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c which really helped for serial setup]
