#!/bin/bash
echo "building project"
make -f makeServer
make -f makeClient
echo "copying client file"
cp clientWithMenu ../testFinal/
echo "done"

