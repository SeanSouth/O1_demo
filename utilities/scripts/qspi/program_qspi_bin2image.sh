#!/bin/bash
base_dir="`dirname "$0"`"
CONFIG_FILE="$base_dir/program_qspi.ini"
CONFIG_SCRIPT="$base_dir/program_qspi_config.sh"
sdkroot=${SDKROOT:=$(pushd $base_dir/../../.. >/dev/null; pwd; popd >/dev/null)}
BIN2IMAGE="$sdkroot/binaries/bin2image"



error() {
        which zenity >/dev/null
        if [ $? -eq 0 ]; then
                zenity --info --title "Programming QSPI Failed" --text "$1"
        else
                echo $1
        fi
}


[ $# -ne 1 ] && echo "Usage: $0 <image>" && exit 1

if [ ! -f $CONFIG_FILE ] ; then
	$CONFIG_SCRIPT
fi

eval $(cat $CONFIG_FILE)


if [ ! -x $BIN2IMAGE ] ; then
        error "bin2mage not found. Please make sure it is built and installed in $(dirname $BIN2IMAGE)"
        exit 1
fi

if [ "$CHIP_REV" = "AC" ] ; then
	echo "Preparing image for AC chip: $1"
	$BIN2IMAGE qspi_cached $1 $1.cached
	exit 0
fi
	

echo "Preparing image for AD chip: $1"

UART=
[ "$ENABLE_UART" = "y" ] && UART=enable_uart

echo $BIN2IMAGE qspi_cached $1 $1.cached AD $UART $RAM_SHUFFLING
$BIN2IMAGE qspi_cached $1 $1.cached AD $UART $RAM_SHUFFLING

