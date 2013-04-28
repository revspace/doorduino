#!/usr/bin/perl -w
use strict;
use POSIX qw(strftime);
use IO::Socket::INET ();
use IO::Handle ();
use Sys::Syslog qw(openlog syslog :macros);

sub slurp ($) { local (@ARGV, $/) = @_; <>; }

my $dev = shift or die "Usage: $0 /dev/ttyUSBx";
(my $devname) = $dev =~ m{ ([^/]+) $ }x;
my $ircname = slurp "ircname.$devname";
chomp $ircname;

$ircname ||= $devname;

system qw(stty -F), $dev, qw(cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost
	-onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke
	noflsh -ixon -crtscts);

open my $in,  "<", $dev or die $!;
open my $out, ">", $dev or die $!;

openlog "doorduino", "nofatal,perror", LOG_USER;
sub logline { syslog LOG_INFO, "@_"; }

my $line;

for (;;) {
    sysread($in, my $char, 1) or next;
    $line .= $char;
    $line =~ /\n$/ or next;

    my $input = $line;
    $line = "";

    logline "Arduino says: $input";

    my ($id) = $input =~ /<(BUTTON|\w{16})>/ or next;

    my $known = join "\n", map slurp($_), glob "ibuttons.acl.d/*.acl";
    my $valid = $known =~ /^$id\b(?:\s+([^\r\n]+))?/mi;
    my $descr = $1;

    if ($valid) {
        logline "Access granted for $descr.\n";
        print {$out} "A\n";
        my $barsay = IO::Socket::INET->new(
            qw(PeerAddr 10.42.42.1  PeerPort 64123  Proto tcp)
        );
        $barsay->print("$ircname unlocked by $descr") if $barsay;
    } else {
        logline "Access denied.\n";
        print {$out} "N\n";
    }
}

