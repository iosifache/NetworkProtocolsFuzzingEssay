#!/bin/bash
while ! ./server
do
  sudo fuser -k 40000/tcp  
  sleep 1
  echo "Restarting program..."
done
