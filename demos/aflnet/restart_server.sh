#!/bin/bash

while ! ./usr/local/bin/civetweb
do
  sudo fuser -k 8080/tcp
  sleep 1
  echo "Restarting program..."
done