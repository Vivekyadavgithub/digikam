/* ============================================================
 * File  : imlibinterface.cpp
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2003-01-15
 * Description : 
 * 
 * Copyright 2003 by Renchi Raju

 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published bythe Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * ============================================================ */

#include <qstring.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qapplication.h>

#include <X11/Xlib.h>
#include <Imlib2.h>

#include <cmath>
#include <cstdio>

#include "imlibinterface.h"

namespace Digikam
{

#define MaxRGB 255L

class ImlibInterfacePrivate {

public:

    Display *display;
    Visual  *vis;
    GC       gc;
    Colormap cm;
    int      depth;

    bool valid;
    bool dirty;
    
    int  width;
    int  height;
    int  origWidth;
    int  origHeight;
    int  selX;
    int  selY;
    int  selW;
    int  selH;
    double zoom;

    float gamma;
    float brightness;
    float contrast;

    QBitmap qmask;
    
    Imlib_Context        context;
    Imlib_Image          image;
    Imlib_Color_Modifier cmod;
    QString              filename;
};

ImlibInterface::ImlibInterface()
    : QObject()
{
    m_instance = this;
    
    d = new ImlibInterfacePrivate;

    d->display = QPaintDevice::x11AppDisplay();

    d->vis   = DefaultVisual(d->display, DefaultScreen(d->display));
    d->depth = DefaultDepth(d->display, DefaultScreen(d->display));
    d->cm    = DefaultColormap(d->display, DefaultScreen(d->display));

    d->context = imlib_context_new();
    imlib_context_push(d->context);
    
    // 60 MB of cache
    imlib_set_cache_size(60 * 1024 * 1024);

    // set the maximum number of colors to allocate for
    // 8bpp and less to 128 
    imlib_set_color_usage(128);
    // dither for depths < 24bpp 
    imlib_context_set_dither(1);
    // set the display , visual, colormap and drawable we are using 
    imlib_context_set_display(d->display);
    imlib_context_set_visual(d->vis);
    imlib_context_set_colormap(d->cm);
    
    d->cmod = imlib_create_color_modifier();
    imlib_context_set_color_modifier(d->cmod);

    imlib_context_pop();

    d->image  = 0;
    d->valid = false;
    d->dirty = true;
    d->width  = 0;
    d->height = 0;
    d->origWidth  = 0;
    d->origHeight = 0;
    d->selX = 0;
    d->selY = 0;
    d->selW = 0;
    d->selH = 0;
    d->zoom = 1.0;
}

ImlibInterface::~ImlibInterface()
{
    delete d;

    m_instance = 0;
}

void ImlibInterface::load(const QString& filename)
{
    d->valid = false;

    d->filename = filename;
    
    imlib_context_push(d->context);
    if (d->image) {
        imlib_context_set_image(d->image);
        imlib_free_image();
        d->image = 0;
    }

    d->width  = 0;
    d->height = 0;
    d->origWidth  = 0;
    d->origHeight = 0;
    d->selX = 0;
    d->selY = 0;
    d->selW = 0;
    d->selH = 0;

    d->image = imlib_load_image(filename.latin1());

    if (d->image) {
        imlib_context_set_image(d->image);

        d->origWidth  = imlib_image_get_width();
        d->origHeight = imlib_image_get_height();
        d->valid  = true;
        d->dirty  = true;

        d->width  = d->origWidth;
        d->height = d->origHeight;

        d->gamma      = 1.0;
        d->contrast   = 1.0;
        d->brightness = 0.0;
    }
    imlib_context_pop();
}

int ImlibInterface::width()
{
    return d->width;    
}

int ImlibInterface::height()
{
    return d->height;
}

int ImlibInterface::origWidth()
{
    return d->origWidth;    
}

int ImlibInterface::origHeight()
{
    return d->origHeight;
}

void ImlibInterface::setSelectedArea(int x, int y, int w, int h)
{
    d->selX = x;
    d->selY = y;
    d->selW = w;
    d->selH = h;
}

void ImlibInterface::getSelectedArea(int& x, int& y, int& w, int& h)
{
    x = d->selX;
    y = d->selY;
    w = d->selW;
    h = d->selH;
}

bool ImlibInterface::hasAlpha()
{
    imlib_context_push(d->context);
    imlib_context_set_image(d->image);

    bool alpha = imlib_image_has_alpha();
    imlib_context_pop();

    return alpha;
}

void ImlibInterface::paint(QPaintDevice* p, int sx, int sy, int sw, int sh,
                           int dx, int dy, int antialias)
{
    if (!d->image)
        return;
    
    imlib_context_push(d->context);
    imlib_context_set_image(d->image);

    bool alpha = false;
    
    imlib_context_set_drawable(p->handle());
    if (imlib_image_has_alpha()) {
        alpha = true;
        if (d->qmask.width()  != d->width ||
            d->qmask.height() != d->height)
            d->qmask.resize(d->width, d->height);
        imlib_context_set_mask(d->qmask.handle());
        imlib_context_set_blend(1);
    }
    else {
        imlib_context_set_mask(0);
        imlib_context_set_blend(0);
    }        
    imlib_context_set_anti_alias(antialias);
    
    int x, y, w, h;
    x = QMAX(int(sx / d->zoom), 0);
    y = QMAX(int(sy / d->zoom), 0);
    w = QMIN(int(sw / d->zoom), d->origWidth);
    h = QMIN(int(sh / d->zoom), d->origHeight);


    // need to set this, bug in imlib2 causes crash
    imlib_context_set_color_modifier(0);

    imlib_render_image_part_on_drawable_at_size(x, y, w, h,
                                                dx, dy, sw, sh);

    imlib_context_pop();
}

void ImlibInterface::paint(QPaintDevice* p, int sx, int sy,
                           int sw, int sh, int dx, int dy,
                           int antialias,
                           int mx, int my, int mw, int mh)
{
    if (!d->image)
        return;

    imlib_context_push(d->context);
    imlib_context_set_image(d->image);

    bool alpha = false;

    imlib_context_set_drawable(p->handle());
    if (imlib_image_has_alpha()) {
        alpha = true;
        if (d->qmask.width()  != d->width ||
            d->qmask.height() != d->height)
            d->qmask.resize(d->width, d->height);
        imlib_context_set_mask(d->qmask.handle());
        imlib_context_set_blend(1);
    }
    else {
        imlib_context_set_mask(0);
        imlib_context_set_blend(0);
    }
    imlib_context_set_anti_alias(antialias);

    int x, y, w, h;
    x = QMAX(int(sx / d->zoom), 0);
    y = QMAX(int(sy / d->zoom), 0);
    w = QMIN(int(sw / d->zoom), d->origWidth);
    h = QMIN(int(sh / d->zoom), d->origHeight);
    
    // need to set this, bug in imlib2 causes crash
    imlib_context_set_color_modifier(0);

    Imlib_Image bot = imlib_create_cropped_scaled_image(x, y, w, h, sw, sh);

    // create the mask -----------------------------------

    Imlib_Image top = imlib_create_image(sw, sh);
    imlib_context_set_image(top);
    DATA32* data = imlib_image_get_data();
    DATA32* ptr = data;

    for (int j=0; j<sh; j++) {
        for (int i=0; i<sw; i++) {
            if (i >= (mx-dx) && i <= (mx-dx+mw-1) &&
                j >= (my-dy) && j <= (my-dy+mh-1))
                *(ptr++) = 0x00000000;
            else
                *(ptr++) = 0xaa000011;
        }
    }
    imlib_image_put_back_data(data);
    imlib_image_set_has_alpha(1);

    // blend the mask -----------------------------------
    imlib_context_set_image(bot);
    imlib_context_set_blend(1);
    imlib_blend_image_onto_image(top, 0, 0, 0, sw, sh,
                                 0, 0, sw, sh);

    imlib_render_image_on_drawable(dx, dy);

    imlib_context_set_image(bot);
    imlib_free_image();
    imlib_context_set_image(top);
    imlib_free_image();

    imlib_context_pop();
}

void ImlibInterface::zoom(double val)
{
    d->zoom   = val;
    d->width  = (int)(d->origWidth  * val);
    d->height = (int)(d->origHeight * val);
    d->dirty  = true;
}

void ImlibInterface::rotate90()
{
    imlib_context_push(d->context);
    imlib_image_orientate(1);
    d->dirty = true;

    d->origWidth = imlib_image_get_width();
    d->origHeight = imlib_image_get_height();
    imlib_context_pop();
}

void ImlibInterface::rotate180()
{
    imlib_context_push(d->context);
    imlib_image_orientate(2);
    d->dirty = true;

    d->origWidth = imlib_image_get_width();
    d->origHeight = imlib_image_get_height();
    imlib_context_pop();
}

void ImlibInterface::rotate270()
{
    imlib_context_push(d->context);
    imlib_image_orientate(3);
    d->dirty = true;

    d->origWidth = imlib_image_get_width();
    d->origHeight = imlib_image_get_height();
    imlib_context_pop();
}

void ImlibInterface::flipHoriz()
{
    imlib_context_push(d->context);
    imlib_image_flip_horizontal();
    d->dirty = true;

    d->origWidth = imlib_image_get_width();
    d->origHeight = imlib_image_get_height();
    imlib_context_pop();
    
}

void ImlibInterface::flipVert()
{
    imlib_context_push(d->context);
    imlib_image_flip_vertical();
    d->dirty = true;

    d->origWidth = imlib_image_get_width();
    d->origHeight = imlib_image_get_height();
    imlib_context_pop();
}

void ImlibInterface::crop(int x, int y, int w, int h)
{
    imlib_context_push(d->context);

    imlib_context_set_image(d->image);
    Imlib_Image im = imlib_create_cropped_image(x, y, w, h);
    imlib_free_image();
    d->image = im;
    imlib_context_set_image(d->image);
    
    d->origWidth = imlib_image_get_width();
    d->origHeight = imlib_image_get_height();
    imlib_context_pop();

    d->dirty = true;
}

void ImlibInterface::resize(int w, int h)
{
    imlib_context_push(d->context);

    imlib_context_set_image(d->image);
    Imlib_Image im =
        imlib_create_cropped_scaled_image(0, 0, d->origWidth, d->origHeight,
                                          w, h);
    imlib_free_image();
    d->image = im;
    imlib_context_set_image(d->image);
    
    d->origWidth = imlib_image_get_width();
    d->origHeight = imlib_image_get_height();
    imlib_context_pop();

    d->dirty = true;
}

void ImlibInterface::restore()
{
    load(d->filename);
}

void ImlibInterface::save(const QString&)
{
}

void ImlibInterface::saveAs(const QString&)
{
    
}

void ImlibInterface::changeGamma(double gamma)
{
    imlib_context_push(d->context);
    imlib_context_set_color_modifier(d->cmod);
    imlib_reset_color_modifier();
    
    d->gamma += gamma/10.0;

    imlib_modify_color_modifier_gamma(d->gamma);
    imlib_modify_color_modifier_brightness(d->brightness);
    imlib_modify_color_modifier_contrast(d->contrast);

    imlib_context_set_color_modifier(0);
    
    d->dirty = true;
    imlib_context_pop();
}

void ImlibInterface::changeBrightness(double brightness)
{
    imlib_context_push(d->context);
    imlib_context_set_color_modifier(d->cmod);
    imlib_reset_color_modifier();

    d->brightness += brightness/100.0;

    imlib_modify_color_modifier_gamma(d->gamma);
    imlib_modify_color_modifier_brightness(d->brightness);
    imlib_modify_color_modifier_contrast(d->contrast);

    imlib_context_set_color_modifier(0);

    d->dirty = true;

    imlib_context_pop();
}

void ImlibInterface::changeContrast(double contrast)
{
    imlib_context_push(d->context);
    imlib_context_set_color_modifier(d->cmod);
    imlib_reset_color_modifier();
    
    d->contrast += contrast/100.0;

    imlib_modify_color_modifier_gamma(d->gamma);
    imlib_modify_color_modifier_brightness(d->brightness);
    imlib_modify_color_modifier_contrast(d->contrast);

    imlib_context_set_color_modifier(0);

    d->dirty = true;

    imlib_context_pop();
}

void ImlibInterface::setBCG(double brightness, double contrast, double gamma)
{
    imlib_context_push(d->context);

    bool alpha = imlib_image_has_alpha();
    
    imlib_context_set_color_modifier(d->cmod);
    imlib_reset_color_modifier();
    
    imlib_modify_color_modifier_brightness(brightness);
    imlib_modify_color_modifier_contrast(contrast);
    imlib_modify_color_modifier_gamma(gamma);
    imlib_apply_color_modifier();

    imlib_reset_color_modifier();
    imlib_context_set_color_modifier(0);

    // restore image's alpha setting.
    // seems applying brightness/contrast/alpha will cause alpha lookup table to
    // change which causes applying the color modifier to make an image have an
    // alpha channel even thought it didn't have one earlier.
    imlib_image_set_has_alpha(alpha ? 1:0);

    imlib_context_pop();

    d->dirty = true;
    emit signalRequestUpdate();
}

uint* ImlibInterface::getData()
{
    if (d->image) {
        imlib_context_push(d->context);
        imlib_context_set_image(d->image);
        DATA32 *ptr = imlib_image_get_data_for_reading_only();
        imlib_context_pop();
        return ptr;
    }
    else
        return 0;
}

void ImlibInterface::putData(uint* data)
{
    imlib_context_push(d->context);
    
    DATA32* ptr = imlib_image_get_data();

    memcpy(ptr, data, d->origWidth*d->origHeight*sizeof(DATA32));
    
    imlib_image_put_back_data(ptr);

    imlib_context_pop();

    d->dirty = true;

    emit signalRequestUpdate();
}

uint* ImlibInterface::getSelectedData()
{
    if (!d->selW || !d->selH)
        return 0;
    
    if (d->image) {
        imlib_context_push(d->context);
        imlib_context_set_image(d->image);

        DATA32 *ptr = imlib_image_get_data_for_reading_only();
        DATA32 *pptr;
        
        DATA32 *data = new DATA32[d->selW*d->selH];
        DATA32 *dptr  = data;

        for (int j = d->selY; j < (d->selY + d->selH); j++) {
            pptr  = &ptr[j*d->origWidth] + d->selX;
            for (int i = 0; i < d->selW; i++) {
                *(dptr++) = *(pptr++);
            }
        }

        imlib_context_pop();

        return data;
    }
    else
        return 0;
}

void ImlibInterface::putSelectedData(uint* data)
{
    if (!data || !d->image)
        return;
    
    imlib_context_push(d->context);
    imlib_context_set_image(d->image);

    DATA32 *ptr = imlib_image_get_data();
    DATA32 *pptr;
        
    DATA32 *dptr  = data;
        
    for (int j = d->selY; j < (d->selY + d->selH); j++) {
        pptr  = &ptr[j*d->origWidth] + d->selX;
        for (int i = 0; i < d->selW; i++) {
            *(pptr++) = *(dptr++);
        }
    }

    imlib_context_pop();

    d->dirty = true;

    emit signalRequestUpdate();
}

ImlibInterface* ImlibInterface::instance()
{
    if (!m_instance) {
        new ImlibInterface();
    }
    
    return m_instance;    
}

ImlibInterface* ImlibInterface::m_instance = 0;

}

#include "imlibinterface.moc"
