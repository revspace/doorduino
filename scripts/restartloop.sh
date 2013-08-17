trap 'echo "--------------"' 2

if [ "$1" == "" ]; then
	echo Usage: $0 /dev/ttyUSBx
	exit 255
fi

while true; do
	sudo chmod 666 $1
	perl doorduino.pl $1
done
