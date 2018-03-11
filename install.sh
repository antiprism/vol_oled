#!/bin/bash

cp vol_oled /usr/local/bin
cp vol_oled.service /etc/systemd/system
systemctl daemon-reload
systemctl enable vol_oled
systemctl start vol_oled

