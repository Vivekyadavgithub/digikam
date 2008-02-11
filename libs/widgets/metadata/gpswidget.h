/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-02-22
 * Description : a tab widget to display GPS info
 * 
 * Copyright (C) 2006-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef GPSWIDGET_H
#define GPSWIDGET_H

// Qt includes.

#include <QWidget>
#include <QString>

// Local includes

#include "metadatawidget.h"
#include "digikam_export.h"

namespace Digikam
{

class GPSWidgetPriv;

class DIGIKAM_EXPORT GPSWidget : public MetadataWidget
{
    Q_OBJECT
    
public:

    enum WebGPSLocator
    {
        MapQuest = 0,
        GoogleMaps,
        MsnMaps,
        MultiMap
    };

public:

    GPSWidget(QWidget* parent, const char* name=0);
    ~GPSWidget();

    bool    loadFromURL(const KUrl& url);
    
    QString getTagDescription(const QString& key);
    QString getTagTitle(const QString& key);

    QString getMetadataTitle(void);
    
    int  getWebGPSLocator(void);
    void setWebGPSLocator(int locator);

protected slots:    
    
    virtual void slotSaveMetadataToFile(void);

private slots:

    void slotGPSDetails(void);

private:

    bool decodeMetadata(void);
    void buildView(void);
    bool decodeGPSPosition(void);
    virtual void setMetadataEmpty();

private:

    GPSWidgetPriv *d;
};

}  // namespace Digikam

#endif /* GPSWIDGET_H */
