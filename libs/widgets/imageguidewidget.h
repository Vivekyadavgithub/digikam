/* ============================================================
 * File  : imageguidewidget.h
 * Author: Gilles Caulier <caulier dot gilles at free.fr>
 * Date  : 2004-08-20
 * Description : 
 * 
 * Copyright 2004-2005 Gilles Caulier
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * ============================================================ */

#ifndef IMAGEGUIDEWIDGET_H
#define IMAGEGUIDEWIDGET_H

// Qt includes.

#include <qwidget.h>
#include <qrect.h>
#include <qpoint.h>
#include <qcolor.h>

// Local includes

#include "digikam_export.h"

class QPixmap;

namespace Digikam
{
class ImageIface;

class DIGIKAM_EXPORT ImageGuideWidget : public QWidget
{
Q_OBJECT

public:

    enum GuideToolMode 
    {
    HVGuideMode=0,
    PickColorMode
    };

public:

    ImageGuideWidget(int w, int h, QWidget *parent=0, 
                     bool spotVisible=true, int guideMode=HVGuideMode,
                     QColor guideColor=Qt::red, int guideSize=1);
    ~ImageGuideWidget();
        
    Digikam::ImageIface* imageIface();
    
    QPoint getSpotPosition(void);
    QColor getSpotColor(void);
    void   setSpotVisible(bool v);
    void   resetSpotPosition(void);

public slots:
        
    void slotChangeGuideColor(const QColor &color);
    void slotChangeGuideSize(int size);    

signals:

    void spotPositionChanged( const QColor &color, bool release, const QPoint &position ); 
    void signalResized(void);  
    
protected:
    
    void paintEvent( QPaintEvent *e );
    void resizeEvent( QResizeEvent * e );
    void timerEvent(QTimerEvent * e);
    void mousePressEvent ( QMouseEvent * e );
    void mouseReleaseEvent ( QMouseEvent * e );
    void mouseMoveEvent ( QMouseEvent * e );
        
private:

    uint                *m_data;
    int                  m_w;
    int                  m_h;
    
    int                  m_timerID;
    int                  m_guideMode;
    int                  m_guideSize;
    int                  m_flicker;

    bool                 m_focus;
    bool                 m_spotVisible;
    
    // Current spot position in preview coordinates.
    QPoint               m_spot;
    
    QRect                m_rect;       
    
    QColor               m_guideColor;
        
    QPixmap             *m_pixmap;
    
    Digikam::ImageIface *m_iface;    
};

}  // NameSpace Digikam

#endif /* IMAGEGUIDEWIDGET_H */
