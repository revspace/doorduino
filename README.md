# Doorduino

## Configuration

Create a configuration file for every door, in which 'dev' points to the
serial device of the associated Arduino. The configuration file has to be valid
Perl syntax, and return a list of key/value pairs.

    echo "dev => '/dev/ttyUSB0'" > /home/pi/frontdoor.conf.pl
    echo "dev => '/dev/ttyUSB1'" > /home/pi/backdoor.conf.pl

Add keys to files in `ibuttons.acl.d`, and optionally add executable scripts to
`granted.d` and `denied.d`.

An example configuration file:

    dev => "/dev/ttyUSB0",
    ircname => "main door",
    allow_insecure => 1,

Valid configuration entries are:

| Key              | Default value    | Description
| ---------------- | ---------------- | -------------
| `dev`            | (mandatory)      | Arduino serial device
| `ircname`        | (config name)    | Display name
| `acldir`         | `ibuttons.acl.d` | Directory in which to look for .acl files
| `allow_insecure` | 0                | Set to a true value (such as 1) to enable ID-only access 

## ID-only support (insecure mode)

By default, only SHA1 secure iButtons (DS1961S) are supported. Some
organizations will have insecure ID-only iButtons (like the DS1990A), and want
to upgrade. In other cases, leaking the shared secret would be worse than
allowing access to the wrong people, and you may want to authenticate against
the ID without having the secret available on the controlling Linux computer
(e.g. Raspberry Pi) if you fear the physical controlling device might get
stolen or broken into.

In these cases, set `allow_insecure => 1`. Note that if there's a secret in the
.acl file, it will still be verified. To disable SHA1 verification of the
shared secret, list the ID without a colon or a secret.

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
