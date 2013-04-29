#!/bin/sh
SSH="ssh -qi sshkey.rsa"

eval $(ssh-agent)
ssh-add sshkey.rsa



for host in 10.42.42.42  10.42.42.43; do
    echo Pushing to $host
    $SSH -t pi@$host 'sudo mount -o rw,remount /' &&
    rsync -e "$SSH" global.acl pi@$host:ibuttons.acl.d/global.acl &&
    $SSH -t pi@$host 'sudo mount -o ro,remount /' ||
    echo "$host failed";
    echo done!
done

eval $(ssh-agent -k)  # kill
