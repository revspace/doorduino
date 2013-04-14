trap 'echo "--------------"' 2

while true; do
	sudo chmod 666 /dev/ttyUSB*
	perl doorduino.pl
done
