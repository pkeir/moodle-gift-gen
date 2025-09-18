#!/bin/bash

URL1="https://upload.wikimedia.org/wikipedia/commons/9/9c/Lunch_atop_a_Skyscraper_-_Charles_Clyde_Ebbets.jpg"
URL2="https://upload.wikimedia.org/wikipedia/commons/c/c4/Apollo_11%2C_1969_%288980505187%29.jpg"

TMPDIR1=$(mktemp -d)
wget -P ${TMPDIR1} ${URL1} # URL decoding %2C in a URL becomes , in the filename
TMPFILE1=$(find ${TMPDIR1} -type f)
IMAGE_FILE1=$(basename ${TMPFILE1})
IMAGE_NAME1="${IMAGE_FILE1%.*}"
mv ${TMPFILE1} .
convert ${IMAGE_FILE1} ${IMAGE_NAME1}.pdf
convert ${IMAGE_FILE1} -resize 800x   -bordercolor white -border 40x40 -gravity north -splice 0x80 -pointsize 24 -annotate +0+20 "John's favourite colour is green.\nJohn's favourite number is 89." text-and-image-test1.pdf

TMPDIR2=$(mktemp -d)
wget -P ${TMPDIR2} ${URL2} # URL decoding %2C in a URL becomes , in the filename
TMPFILE2=$(find ${TMPDIR2} -type f)
IMAGE_FILE2=$(basename ${TMPFILE2})
IMAGE_NAME2="${IMAGE_FILE2%.*}"
mv ${TMPFILE2} .
convert ${IMAGE_FILE2} ${IMAGE_NAME2}.pdf
convert ${IMAGE_FILE2} -resize 800x   -bordercolor white -border 40x40 -gravity north -splice 0x80 -pointsize 24 -annotate +0+20 "Emma's favourite animal is a penguin.\nEmma's favourite country is Scotland." text-and-image-test2.pdf

