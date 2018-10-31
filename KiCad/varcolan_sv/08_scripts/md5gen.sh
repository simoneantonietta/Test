#!/bin/bash
md5sum raspbian-varcolansv.img > md5sum.txt
DEV=$1
sed -i -e "s/raspbian-varcolansv.img/\/dev\/$DEV/g" md5sum.txt

echo "DONE"
