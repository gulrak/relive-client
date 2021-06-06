#!/bin/sh
convert "$1" -resize 550x +repage \( +clone -background black -shadow 80x20+0+15 \) +swap -background transparent -layers merge +repage "$2"
