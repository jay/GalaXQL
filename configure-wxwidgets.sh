#! bash
###########################################################################
# This file is a reminder of how I built wxwidgets on Windows.
#
# Use the script like:
#   bash
#   cd <wxwidgets_top_dir>
#   <path_to_this_script>/configure-wxwidgets.sh
#   gmake clean all
#   gmake -C contrib/src/stc clean all
#
###########################################################################

export PATH=/usr/bin:$PATH
export MAKE=gmake

if [ -f configarg.cache ];
then
  rm configarg.cache
fi

if [ "$DEBUG" == "1" ];
then
  export ADDITIONAL_OPTIONS="--enable-debug --enable-debug_gdb --enable-debug_info --disable-optimise"
else
  export ADDITIONAL_OPTIONS="--disable-debug --enable-optimise"
fi

if [ "$PROFILE" == "1" ];
then
export ADDITIONAL_OPTIONS="$ADDITIONAL_OPTIONS --enable-profile"
fi

./configure                       \
    $ADDITIONAL_OPTIONS           \
    --with-msw                    \
    --disable-rtti                \
    --with-opengl=yes             \
    --with-expat=no               \
    --with-regex=no               \
    --with-libjpeg=no             \
    --with-libtiff=no             \
    --enable-monolithic           \
    --disable-shared              \
    --disable-statusbar           \
    --disable-catch_segvs         \
    --disable-toolbar             \
    --disable-sound               \
    --disable-mediactrl           \
    --disable-help                \
    --disable-printarch           \
    --disable-aui                 \
    --disable-toolbook            \
    --disable-joystick            \
    --disable-splash              \
    --disable-tooltips            \
    --disable-sockets             \
    --disable-aboutdlg            \
    --disable-protocol            \
    --disable-protocols           \
    --disable-protocol-http       \
    --disable-protocol-ftp        \
    --disable-protocol-file       \
    --disable-ftp                 \
    --disable-http                \
    --disable-fileproto           \
    --disable-fs_inet             \
    --disable-url                 \
    --disable-tarstream           \
    --disable-mdi                 \
    --disable-tipdlg              \
    --disable-tipwindow           \
    --disable-largefile           \
    --disable-animatectrl         \
    --disable-dynamicloader       \
    --disable-display             \
    --disable-tga                 \
    --disable-pcx                 \
    --disable-iff                 \
    --disable-pnm                 \
    --disable-gif                 \
    --disable-xrc                 \
    --disable-wizarddlg           \
    --disable-threads             \
    --disable-docview             \
    --disable-help                \
    --disable-metafile            \
    --disable-dnd                 \
    --disable-dragimage           \
    --disable-miniframe           \
    --disable-calendar            \
    --disable-datepick            \
    --disable-logwindow           \
    --disable-logdialog           \
    --disable-listbook            \
    --disable-numberdlg           \
    --disable-progressdlg         \
    --disable-postscript          \
    --disable-cmdline             \
    --disable-mimetype            \
    --disable-splines             \
    --disable-unicode             \
    --disable-sysoptions          \
    --enable-no_deps              \
    --disable-dependency-tracking \
    --host=i686-pc-mingw32        \
