#!/bin/bash
    cd ./mwtest
    ./wtbyte $1
    ./encode
    cd ../mrtest
    ./read $1
    cd ..
    
