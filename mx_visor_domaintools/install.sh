#!/bin/bash

mkdir /usr/bin/ressource_load
cp ressource_load.sh /usr/bin/ressource_load/
cp ressource_load.service /etc/systemd/system/
systemctl daemon-reload
systemctl start ressource_load.service

