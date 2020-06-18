#!/bin/sh
sudo LD_LIBRARY_PATH=`pwd`/bin:$LD_LIBRARY_PATH runuser -u www-data ./listen.sh

