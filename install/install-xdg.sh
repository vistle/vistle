#! /bin/bash

mode=user
if [ "$EUID" -eq 0 ]; then
    mode=system
fi

xdg-mime install --mode $mode hlrs-vsl.desktop

xdg-desktop-menu install --mode $mode --noupdate hlrs-vistle.desktop
xdg-desktop-menu install --mode $mode --noupdate hlrs-vistle_opener.desktop
xdg-desktop-menu forceupdate --mode $mode
xdg-mime default hlrs-vistle_opener.desktop x-scheme-handler/vistle

xdg-icon-resource install --mode $mode --noupdate --size 48 hlrs-vistle.png
xdg-icon-resource install --mode $mode --noupdate --context mimetypes --size 48 hlrs-vistle.png text/x-vsl
xdg-icon-resource forceupdate --mode $mode
