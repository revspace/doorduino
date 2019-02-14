# Doorduino

## Configuration

Create a configuration file for every door, in which 'dev' points to the
serial device of the associated Arduino.

    echo "dev => '/dev/ttyUSB0'" > /home/pi/frontdoor.conf.pl
    echo "dev => '/dev/ttyUSB1'" > /home/pi/backdoor.conf.pl

Add keys to files in `ibuttons.acl.d`, and optionally add executable scripts to
`granted.d` and `denied.d`.

## Starting manually, in the foreground

    ./doorduino.pl frontdoor  # or another config file

## Starting automatically using the systemd unit

    cp doorduino@.service /etc/systemd/system

    systemctl enable doorduino@frontdoor
    systemctl enable doorduino@backdoor

    systemctl status doorduino@frontdoor

## Starting automatically with cron

See `doc/crontab.example` and `scripts/restartloop.sh`

## See also

https://revspace.nl/Doorduino
