#!/bin/sh

echo Configuring MineFlags for unix..
echo "# generated by fix.sh" > ./Makefile
echo "include Makefile.nix" >> ./Makefile
echo Done!
