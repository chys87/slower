#!/bin/sh

slower="$0"

usage() {
	echo "$slower [-f SLOWER_FACTOR] cmd ..."
	echo "SLOWER_FACTOR is between 0.01 and 100"
}

SLOWER_FACTOR=1

while [ $# -ge 1 ]; do
	case "$1" in
		-f)
			SLOWER_FACTOR="$2"
			shift
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			break
			;;
	esac
done

if [ $# -eq 0 ]; then
	usage
	exit 1
fi

export SLOWER_FACTOR
export LD_PRELOAD="$HOME/.local/lib64/libslower.so${LD_PRELOAD+:$LD_PRELOAD}"

exec "$@"
