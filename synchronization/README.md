> Here's a quick guide on how to configure the RPi master/slave unit after having written the latest image onto the sd-card:
> 
> Username      : alarm
> Password      : alarm
> Root password : root
> Master IP     : 192.168.1.13
> Slave IP      : 192.168.1.14
> 
> 1. On both:
> 
>  - copy setup folder to user's home folder on pi (replace by pi's IP)
>    (on pc) $ scp -r ./setup alarm@192.168.1.50:/home/alarm/
>    (on pi) $ su
>    (on pi) # cp -r /home/alarm/setup/* /
> 
> 2a. On master:
>  - become root:
>    $ su
> 
>  - change hostname in /etc/hostname to rpimaster
>    # echo rpimaster > /etc/hostname
> 
>  - setup master static IP address
>    # systemctl disable systemd-networkd.service
>    # netctl enable eth0master
> 
>  - setup time synchronization master daemon:
>    # systemctl disable systemd-timesyncd.service
>    # systemctl daemon-reload
>    # systemctl enable ptpd-master.service
> 
>  - reboot:
>    # reboot
> 
> 2b. On slave:
>  - become root:
>    $ su
> 
>  - change hostname in /etc/hostname to rpislave
>    # echo rpislave > /etc/hostname
> 
>  - setup slave static IP address
>    # systemctl disable systemd-networkd.service
>    # netctl enable eth0slave
> 
>  - setup time synchronization slave daemon:
>    # systemctl disable systemd-timesyncd.service
>    # systemctl daemon-reload
>    # systemctl enable ptpd-slave.service
> 
>  - reboot:
>    # reboot
> 
> 3. On both:
> 
>  - start virtual python environment:
>    $ source ~/python2/bin/activate (for python 2)
>    $ source ~/python3/bin/activate (for python 3)
>    (Type deactivate to exit environment)
