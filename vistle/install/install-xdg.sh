xdg-mime install hlrs-vsl.desktop

xdg-desktop-menu install --noupdate hlrs-vistle.desktop
xdg-desktop-menu install --noupdate hlrs-vistle_opener.desktop
xdg-desktop-menu forceupdate
xdg-mime default hlrs-vistle_opener.desktop x-scheme-handler/vistle

xdg-icon-resource install --noupdate --size 48 hlrs-vistle.png
xdg-icon-resource install --noupdate --context mimetypes --size 48 hlrs-vistle.png text/x-vsl
xdg-icon-resource forceupdate
