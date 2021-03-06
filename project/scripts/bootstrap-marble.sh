#!/bin/sh

# Copyright (c) 2008-2019, Gilles Caulier, <caulier dot gilles at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Copy this script on root folder where are source code

export PATH=$QTDIR/bin:$PATH
export PKG_CONFIG_PATH=/usr/lib/pkgconfig

#export VERBOSE=1

# We will work on command line using MinGW compiler
export MAKEFILES_TYPE='Unix Makefiles'

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

export MarbleOptions='-DWITH_DESIGNER_PLUGIN=OFF \
                      -DBUILD_MARBLE_TESTS=OFF \
                      -DBUILD_MARBLE_TOOLS=OFF \
                      -DBUILD_MARBLE_EXAMPLES=OFF \
                      -DBUILD_MARBLE_APPS=OFF \
                      -DBUILD_WITH_DBUS=OFF \
                      -DQTONLY=ON \
                      -DBUILD_TESTING=OFF'

cmake -G "$MAKEFILES_TYPE" . \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -Wno-dev \
      $MarbleOptions \
      ..

