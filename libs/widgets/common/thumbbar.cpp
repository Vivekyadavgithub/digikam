/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-11-22
 * Description : a bar widget to display image thumbnails
 *
 * Copyright (C) 2004-2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2005-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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


#include "thumbbar.h"
#include "thumbbar.moc"

// C++ includes.

#include <cmath>

// Qt includes.

#include <QDateTime>
#include <QDir>
#include <QFrame>
#include <QHash>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QTextDocument>
#include <QTimer>
#include <QToolTip>

// KDE includes.

#include <kapplication.h>
#include <kcodecs.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>

// LibKDcraw includes.

#include <libkdcraw/kdcraw.h>
#include <libkdcraw/version.h>

#if KDCRAW_VERSION < 0x000400
#include <libkdcraw/dcrawbinary.h>
#endif

// Local includes.

#include "dmetadata.h"
#include "thumbnailloadthread.h"
#include "thumbnailsize.h"

namespace Digikam
{

class ThumbBarViewPriv
{
public:

    ThumbBarViewPriv() :
        margin(5)
    {
        dragging        = false;
        clearing        = false;
        needPreload     = false;
        toolTip         = 0;
        firstItem       = 0;
        lastItem        = 0;
        currItem        = 0;
        count           = 0;
        thumbLoadThread = 0;
        tileSize        = ThumbnailSize::Small;
        maxTileSize     = 256;
    }

    bool                        clearing;
    bool                        dragging;
    bool                        needPreload;

    const int                   margin;
    int                         count;
    int                         tileSize;
    int                         orientation;
    int                         maxTileSize;

    QTimer                     *timer;
    QTimer                     *preloadTimer;

    QPoint                      dragStartPos;

    ThumbBarItem               *firstItem;
    ThumbBarItem               *lastItem;
    ThumbBarItem               *currItem;

    QHash<KUrl, ThumbBarItem*>  itemHash;
    ThumbnailLoadThread        *thumbLoadThread;

    ThumbBarToolTipSettings     toolTipSettings;

    ThumbBarToolTip            *toolTip;
};

// -------------------------------------------------------------------------

class ThumbBarItemPriv
{
public:

    ThumbBarItemPriv()
    {
        pos    = 0;
        next   = 0;
        prev   = 0;
        view   = 0;
    }

    int           pos;

    KUrl          url;

    ThumbBarItem *next;
    ThumbBarItem *prev;

    ThumbBarView *view;
};

// -------------------------------------------------------------------------

ThumbBarView::ThumbBarView(QWidget* parent, int orientation, bool exifRotate,
                           ThumbBarToolTipSettings settings)
            : Q3ScrollView(parent)
{
    d = new ThumbBarViewPriv;
    d->orientation     = orientation;
    d->toolTipSettings = settings;
    d->toolTip         = new ThumbBarToolTip(this);
    d->timer           = new QTimer(this);
    d->preloadTimer    = new QTimer(this);
    d->preloadTimer->setSingleShot(true);
    d->thumbLoadThread = ThumbnailLoadThread::defaultThumbBarThread();
    d->thumbLoadThread->setExifRotate(exifRotate);
    d->maxTileSize     = d->thumbLoadThread->maximumThumbnailSize();

    connect(d->thumbLoadThread, SIGNAL(signalThumbnailLoaded(const LoadingDescription&, const QPixmap&)),
            this, SLOT(slotGotThumbnail(const LoadingDescription&, const QPixmap&)));

    connect(d->timer, SIGNAL(timeout()),
            this, SLOT(slotUpdate()));

    connect(d->preloadTimer, SIGNAL(timeout()),
            this, SLOT(slotPreload()));

    connect(this, SIGNAL(contentsMoving(int,int)),
            this, SLOT(slotContentsMoved()));

    viewport()->setMouseTracking(true);
    viewport()->setAcceptDrops(true);

    setFrameStyle(QFrame::NoFrame);
    setAcceptDrops(true);

    if (d->orientation == Qt::Vertical)
    {
        setMaximumWidth(d->maxTileSize + 2*d->margin + verticalScrollBar()->sizeHint().width());
        setHScrollBarMode(Q3ScrollView::AlwaysOff);
    }
    else
    {
        setMaximumHeight(d->maxTileSize + 2*d->margin + horizontalScrollBar()->sizeHint().height());
        setVScrollBarMode(Q3ScrollView::AlwaysOff);
    }
}

ThumbBarView::~ThumbBarView()
{
    // Delete all hash items
    while (!d->itemHash.isEmpty())
    {
        ThumbBarItem *value = *d->itemHash.begin();
        d->itemHash.erase(d->itemHash.begin());
        delete value;
    }

    clear(false);

    delete d->timer;
    delete d->toolTip;
    delete d;
}

void ThumbBarView::resizeEvent(QResizeEvent* e)
{
    if (!e) return;

    Q3ScrollView::resizeEvent(e);

    if (d->orientation == Qt::Vertical)
    {
        d->tileSize = width() - 2*d->margin - verticalScrollBar()->sizeHint().width();
        verticalScrollBar()->setSingleStep(d->tileSize);
        verticalScrollBar()->setPageStep(2*d->tileSize);
    }
    else
    {
        d->tileSize = height() - 2*d->margin - horizontalScrollBar()->sizeHint().height();
        horizontalScrollBar()->setSingleStep(d->tileSize);
        horizontalScrollBar()->setPageStep(2*d->tileSize);
    }

    rearrangeItems();
    ensureItemVisible(currentItem());
}

void ThumbBarView::setExifRotate(bool exifRotate)
{
    if (d->thumbLoadThread->exifRotate() == exifRotate)
        return;

    d->thumbLoadThread->setExifRotate(exifRotate);

    for (ThumbBarItem *item = d->firstItem; item; item = item->d->next)
        invalidateThumb(item);

    triggerUpdate();
}

bool ThumbBarView::getExifRotate()
{
    return d->thumbLoadThread->exifRotate();
}

int ThumbBarView::getOrientation()
{
    return d->orientation;
}

int ThumbBarView::getTileSize()
{
    return d->tileSize;
}

int ThumbBarView::getMargin()
{
    return d->margin;
}

void ThumbBarView::setToolTipSettings(const ThumbBarToolTipSettings &settings)
{
    d->toolTipSettings = settings;
}

ThumbBarToolTipSettings& ThumbBarView::getToolTipSettings()
{
    return d->toolTipSettings;
}

int ThumbBarView::countItems()
{
    return d->count;
}

KUrl::List ThumbBarView::itemsUrls()
{
    KUrl::List urlList;
    if (!countItems())
        return urlList;

    for (ThumbBarItem *item = firstItem(); item; item = item->next())
        urlList.append(item->url());

    return urlList;
}

void ThumbBarView::triggerUpdate()
{
    d->timer->setSingleShot(true);
    d->timer->start(0);
}

ThumbBarItem* ThumbBarView::currentItem() const
{
    return d->currItem;
}

ThumbBarItem* ThumbBarView::firstItem() const
{
    return d->firstItem;
}

ThumbBarItem* ThumbBarView::lastItem() const
{
    return d->lastItem;
}

ThumbBarItem* ThumbBarView::findItem(const QPoint& pos) const
{
    int itemPos;

    if (d->orientation == Qt::Vertical)
        itemPos = pos.y();
    else
        itemPos = pos.x();

    for (ThumbBarItem *item = d->firstItem; item; item = item->d->next)
    {
        if (itemPos >= item->d->pos && itemPos <= (item->d->pos+d->tileSize+2*d->margin))
        {
            return item;
        }
    }

    return 0;
}

ThumbBarItem* ThumbBarView::findItemByUrl(const KUrl& url) const
{
    for (ThumbBarItem *item = d->firstItem; item; item = item->d->next)
    {
        if (item->url().equals(url))
        {
            return item;
        }
    }

    return 0;
}

void ThumbBarView::setSelected(ThumbBarItem* item)
{
    if (!item) return;

    ensureItemVisible(item);
    emit signalUrlSelected(item->url());
    emit signalItemSelected(item);

    if (d->currItem == item) return;

    if (d->currItem)
    {
        ThumbBarItem* item = d->currItem;
        d->currItem = 0;
        item->repaint();
    }

    d->currItem = item;
    if (d->currItem)
        item->repaint();
}

void ThumbBarView::ensureItemVisible(ThumbBarItem* item)
{
    if (item)
    {
        int pos = (int)item->d->pos + d->margin + d->tileSize*.5;

        // We want the complete thumb visible and the next one.
        // find the middle of the image and give a margin of 1,5 image
        // When changed, watch regression for bug 104031
        if (d->orientation == Qt::Vertical)
            ensureVisible(0, pos, 0, viewport()->height());
        else
            ensureVisible(pos, 0, viewport()->width(), 0);
    }
}

void ThumbBarView::refreshThumbs(const KUrl::List& urls)
{
    for (KUrl::List::const_iterator it = urls.begin() ; it != urls.end() ; ++it)
    {
        ThumbBarItem *item = findItemByUrl(*it);
        if (item)
        {
            invalidateThumb(item);
        }
    }
}

void ThumbBarView::invalidateThumb(ThumbBarItem* item)
{
    if (!item) return;

    d->thumbLoadThread->deleteThumbnail(item->url().path());
    d->thumbLoadThread->find(item->url().path(), d->tileSize);
}

bool ThumbBarView::pixmapForItem(ThumbBarItem *item, QPixmap &pix) const
{
    if (d->tileSize > d->maxTileSize)
    {
        //TODO: Install a widget maximum size to prevent this situation
        bool hasPixmap = d->thumbLoadThread->find(item->url().path(), pix, d->maxTileSize);
        if (hasPixmap)
        {
            kWarning(50003) << "Thumbbar: Requested thumbnail size" << d->tileSize
                            << "is larger than the maximum thumbnail size" << d->maxTileSize
                            << ". Returning a scaled-up image." << endl;
            pix = pix.scaled(d->tileSize, d->tileSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return true;
        }
        else
            return false;
    }
    else
    {
        return d->thumbLoadThread->find(item->url().path(), pix, d->tileSize);
    }
}

void ThumbBarView::preloadPixmapForItem(ThumbBarItem *item) const
{
    d->thumbLoadThread->preload(item->url().path(), qMin(d->tileSize, d->maxTileSize));
}

void ThumbBarView::viewportPaintEvent(QPaintEvent* e)
{
    int ts;
    QRect tile;
    QRect contentsPaintRect(viewportToContents(e->rect().topLeft()), viewportToContents(e->rect().bottomRight()));

    if (d->orientation == Qt::Vertical)
    {
       ts = d->tileSize + 2*d->margin;
       tile = QRect(0, 0, visibleWidth(), ts);
    }
    else
    {
       ts = d->tileSize + 2*d->margin;
       tile = QRect(0, 0, ts, visibleHeight());
    }

    QPainter p(viewport());
    p.fillRect(e->rect(), palette().color(QPalette::Background));

    for (ThumbBarItem *item = d->firstItem; item; item = item->d->next)
    {
        if (d->orientation == Qt::Vertical)
        {
            if (item->rect().intersects(contentsPaintRect))
            {
                int translate = item->d->pos - contentsY();
                p.translate(0, translate);

                p.setPen(Qt::white);
                if (item == d->currItem)
                    p.setBrush(palette().highlight().color());
                else
                    p.setBrush(palette().background().color());

                p.drawRect(tile);

                QPixmap pix;
                if (pixmapForItem(item, pix))
                {
                    //QPixmap pix = item->pixmap().scaled(d->tileSize, d->tileSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = (tile.width()  - pix.width())/2;
                    int y = (tile.height() - pix.height())/2;
                    p.drawPixmap(x, y, pix);
                }

                p.translate(0, - translate);
            }
        }
        else
        {
            if (item->rect().intersects(contentsPaintRect))
            {
                int translate = item->d->pos - contentsX();
                p.translate(translate, 0);

                p.setPen(Qt::white);
                if (item == d->currItem)
                    p.setBrush(palette().highlight().color());
                else
                    p.setBrush(palette().background().color());

                p.drawRect(tile);

                QPixmap pix;
                if (pixmapForItem(item, pix))
                {
                    int x = (tile.width()  - pix.width())/2;
                    int y = (tile.height() - pix.height())/2;
                    p.drawPixmap(x, y, pix);
                }

                p.translate(- translate, 0);
            }
        }
    }

    checkPreload();
}

void ThumbBarView::contentsMousePressEvent(QMouseEvent* e)
{
    ThumbBarItem* barItem = findItem(e->pos());
    d->dragging           = true;
    d->dragStartPos       = e->pos();

    if (!barItem || barItem == d->currItem)
        return;

    if (d->currItem)
    {
        ThumbBarItem* item = d->currItem;
        d->currItem = 0;
        item->repaint();
    }

    d->currItem = barItem;
    barItem->repaint();
}

void ThumbBarView::contentsMouseMoveEvent(QMouseEvent *e)
{
    if (!e) return;

    if (d->dragging && (e->button() & Qt::LeftButton))
    {
        if ( findItem(d->dragStartPos) &&
             (d->dragStartPos - e->pos()).manhattanLength() > QApplication::startDragDistance() )
        {
            startDrag();
        }
        return;
    }
}

void ThumbBarView::contentsMouseReleaseEvent(QMouseEvent *e)
{
    d->dragging = false;
    ThumbBarItem *item = findItem(e->pos());
    if (item)
    {
        emit signalUrlSelected(item->url());
        emit signalItemSelected(item);
    }
}

void ThumbBarView::contentsWheelEvent(QWheelEvent *e)
{
    e->accept();

    if (e->delta() < 0)
    {
        if (e->modifiers() & Qt::ShiftModifier)
        {
            if (d->orientation == Qt::Vertical)
                scrollBy(0, verticalScrollBar()->pageStep());
            else
                scrollBy(horizontalScrollBar()->pageStep(), 0);
        }
        else
        {
            if (d->orientation == Qt::Vertical)
                scrollBy(0, verticalScrollBar()->singleStep());
            else
                scrollBy(horizontalScrollBar()->singleStep(), 0);
        }
    }

    if (e->delta() > 0)
    {
        if (e->modifiers() & Qt::ShiftModifier)
        {
            if (d->orientation == Qt::Vertical)
                scrollBy(0, (-1)*verticalScrollBar()->pageStep());
            else
                scrollBy((-1)*horizontalScrollBar()->pageStep(), 0);
        }
        else
        {
            if (d->orientation == Qt::Vertical)
                scrollBy(0, (-1)*verticalScrollBar()->singleStep());
            else
                scrollBy((-1)*horizontalScrollBar()->singleStep(), 0);
        }
    }
}

void ThumbBarView::startDrag()
{
}

void ThumbBarView::clear(bool updateView)
{
    d->clearing = true;

    ThumbBarItem *item = d->firstItem;
    while (item)
    {
        ThumbBarItem *tmp = item->d->next;
        delete item;
        item = tmp;
    }

    d->firstItem = 0;
    d->lastItem  = 0;
    d->count     = 0;
    d->currItem  = 0;

    if (updateView)
        slotUpdate();

    d->clearing = false;

    emit signalItemSelected(0);
}

void ThumbBarView::insertItem(ThumbBarItem* item)
{
    if (!item) return;

    if (!d->firstItem)
    {
        d->firstItem = item;
        d->lastItem  = item;
        item->d->prev = 0;
        item->d->next = 0;
    }
    else
    {
        d->lastItem->d->next = item;
        item->d->prev = d->lastItem;
        item->d->next = 0;
        d->lastItem = item;

    }

    if (!d->currItem)
    {
        d->currItem = item;
        emit signalUrlSelected(item->url());
        emit signalItemSelected(item);
    }

    d->itemHash.insert(item->url(), item);

    d->count++;
    triggerUpdate();
    emit signalItemAdded();
}

void ThumbBarView::takeItem(ThumbBarItem* item)
{
    if (!item) return;

    d->count--;

    if (item == d->firstItem)
    {
        d->firstItem = d->currItem = d->firstItem->d->next;
        if (d->firstItem)
            d->firstItem->d->prev = 0;
        else
            d->firstItem = d->lastItem = d->currItem = 0;
    }
    else if (item == d->lastItem)
    {
        d->lastItem = d->currItem = d->lastItem->d->prev;
        if ( d->lastItem )
           d->lastItem->d->next = 0;
        else
            d->firstItem = d->lastItem = d->currItem = 0;
    }
    else
    {
        ThumbBarItem *i = item;
        if (i)
        {
            if (i->d->prev )
            {
                i->d->prev->d->next = d->currItem = i->d->next;
            }
            if ( i->d->next )
            {
                i->d->next->d->prev = d->currItem = i->d->prev;
            }
        }
    }

    d->itemHash.remove(item->url());

    if (!d->clearing)
        triggerUpdate();

    if (d->count == 0)
        emit signalItemSelected(0);
}

void ThumbBarView::removeItem(ThumbBarItem* item)
{
    if (!item) return;
    delete item;
}

void ThumbBarView::rearrangeItems()
{
    KUrl::List urlList;

    int pos = 0;
    ThumbBarItem *item = d->firstItem;

    while (item)
    {
        item->d->pos = pos;
        pos += d->tileSize + 2*d->margin;
        item = item->d->next;
    }

    if (d->orientation == Qt::Vertical)
        resizeContents(visibleWidth(), d->count*(d->tileSize+2*d->margin));
    else
        resizeContents(d->count*(d->tileSize+2*d->margin), visibleHeight());

    // only trigger preload if we have valid arranged items
    if (d->count)
        d->needPreload = true;
}

void ThumbBarView::repaintItem(ThumbBarItem* item)
{
    if (item)
    {
       if (d->orientation == Qt::Vertical)
           repaintContents(0, item->d->pos, visibleWidth(), d->tileSize+2*d->margin);
       else
           repaintContents(item->d->pos, 0, d->tileSize+2*d->margin, visibleHeight());
    }
}

void ThumbBarView::slotUpdate()
{
    rearrangeItems();
    viewport()->update();
}

void ThumbBarView::checkPreload()
{
    if (d->needPreload && !d->preloadTimer->isActive())
        d->preloadTimer->start(50);
}

void ThumbBarView::slotContentsMoved()
{
    d->needPreload = true;
}

void ThumbBarView::slotPreload()
{
    d->needPreload = false;
    // we get items in an area visibleWidth() to the left and right of the visible area
    QRect visibleArea(contentsX(), contentsY(), visibleWidth(), visibleHeight());

    if (getOrientation() == Qt::Vertical)
    {
        int y1 = contentsY() - visibleHeight();
        int y2 = contentsY();
        int y3 = contentsY() + visibleHeight();
        int y4 = contentsY() + 2* visibleHeight();

        for (ThumbBarItem *item = firstItem(); item; item = item->next())
        {
            int pos = item->position();
            if ( (y1 <= pos && pos <= y2) || (y3 <= pos && pos <= y4))
            {
                if (!item->rect().intersects(visibleArea))
                    preloadPixmapForItem(item);
            }

            if (pos > y4)
                break;
        }
    }
    else
    {
        int x1 = contentsX() - visibleWidth();
        int x2 = contentsX();
        int x3 = contentsX() + visibleWidth();
        int x4 = contentsX() + 2* visibleWidth();

        for (ThumbBarItem *item = firstItem(); item; item = item->next())
        {
            int pos = item->position();
            if ( (x1 <= pos && pos <= x2) || (x3 <= pos && pos <= x4))
            {
                if (!item->rect().intersects(visibleArea))
                    preloadPixmapForItem(item);
            }

            if (pos > x4)
                break;
        }
    }
}

void ThumbBarView::slotGotThumbnail(const LoadingDescription& desc, const QPixmap& pix)
{
    if (!pix.isNull())
    {
        QHash<KUrl, ThumbBarItem*>::const_iterator it = d->itemHash.find(KUrl(desc.filePath));
        if (it == d->itemHash.end())
            return;

        ThumbBarItem* item = *it;
        item->repaint();
    }
}

bool ThumbBarView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent *helpEvent = dynamic_cast<QHelpEvent *>(event);
        if (helpEvent)
        {
            QString tipText;
            QRect rect = toolTip()->maybeTip(helpEvent->pos(), tipText);
            if (!rect.isEmpty())
                QToolTip::showText(helpEvent->globalPos(), tipText, this);
            else
                QToolTip::hideText();
        }
    }

    return QWidget::event(event);
}

ThumbBarToolTip* ThumbBarView::toolTip() const
{
    return d->toolTip;
}

// -------------------------------------------------------------------------

ThumbBarItem::ThumbBarItem(ThumbBarView* view, const KUrl& url)
{
    d = new ThumbBarItemPriv;
    d->url  = url;
    d->view = view;
    d->view->insertItem(this);
}

ThumbBarItem::~ThumbBarItem()
{
    d->view->takeItem(this);
    delete d;
}

KUrl ThumbBarItem::url() const
{
    return d->url;
}

ThumbBarItem* ThumbBarItem::next() const
{
    return d->next;
}

ThumbBarItem* ThumbBarItem::prev() const
{
    return d->prev;
}

QRect ThumbBarItem::rect() const
{
    if (d->view->d->orientation == Qt::Vertical)
    {
        return QRect(0, d->pos,
                     d->view->visibleWidth(),
                     d->view->d->tileSize + 2*d->view->d->margin);
    }
    else
    {
        return QRect(d->pos, 0,
                     d->view->d->tileSize + 2*d->view->d->margin,
                     d->view->visibleHeight());
    }
}

int ThumbBarItem::position() const
{
    return d->pos;
}

void ThumbBarItem::repaint()
{
    d->view->repaintItem(this);
}

// -------------------------------------------------------------------------

ThumbBarToolTip::ThumbBarToolTip(ThumbBarView* parent)
               : m_maxStringLen(30), m_view(parent)
{
    m_headBeg = QString("<tr bgcolor=\"#73CAE6\"><td colspan=\"2\">"
                        "<nobr><font size=\"-1\" color=\"black\"><b>");
    m_headEnd = QString("</b></font></nobr></td></tr>");

    m_cellBeg = QString("<tr><td><nobr><font size=\"-1\" color=\"black\">");
    m_cellMid = QString("</font></nobr></td>"
                        "<td><nobr><font size=\"-1\" color=\"black\">");
    m_cellEnd = QString("</font></nobr></td></tr>");

    m_cellSpecBeg = QString("<tr><td><nobr><font size=\"-1\" color=\"black\">");
    m_cellSpecMid = QString("</font></nobr></td>"
                            "<td><nobr><font size=\"-1\" color=\"steelblue\"><i>");
    m_cellSpecEnd = QString("</i></font></nobr></td></tr>");
}

ThumbBarToolTip::~ThumbBarToolTip()
{
}

QRect ThumbBarToolTip::maybeTip(const QPoint& pos, QString& tipText)
{
    if (!m_view) return QRect();

    ThumbBarItem* item = m_view->findItem( m_view->viewportToContents(pos) );
    if (!item) return QRect();

    if (!m_view->getToolTipSettings().showToolTips) return QRect();

    tipText = tipContents(item);
    tipText.append("</table>");

    return item->rect();
}

QString ThumbBarToolTip::tipContents(ThumbBarItem* item) const
{
    ThumbBarToolTipSettings settings = m_view->getToolTipSettings();

    QString tipText, str;
    QString unavailable(i18n("unavailable"));

    tipText = "<table cellspacing=\"0\" cellpadding=\"0\" width=\"250\" border=\"0\">";

    QFileInfo fileInfo(item->url().path());
    KFileItem fi(KFileItem::Unknown, KFileItem::Unknown, item->url());
    DMetadata metaData(item->url().path());

    // -- File properties ----------------------------------------------

    if (settings.showFileName  ||
        settings.showFileDate  ||
        settings.showFileSize  ||
        settings.showImageType ||
        settings.showImageDim)
    {
        tipText += m_headBeg + i18n("File Properties") + m_headEnd;

        if (settings.showFileName)
        {
            tipText += m_cellBeg + i18n("Name:") + m_cellMid;
            tipText += item->url().fileName() + m_cellEnd;
        }

        if (settings.showFileDate)
        {
            QDateTime modifiedDate = fileInfo.lastModified();
            str = KGlobal::locale()->formatDateTime(modifiedDate, KLocale::ShortDate, true);
            tipText += m_cellBeg + i18n("Modified:") + m_cellMid + str + m_cellEnd;
        }

        if (settings.showFileSize)
        {
            tipText += m_cellBeg + i18n("Size:") + m_cellMid;
            str = i18n("%1 (%2)", KIO::convertSize(fi.size()),
                                  KGlobal::locale()->formatNumber(fi.size(),
                                  0));
            tipText += str + m_cellEnd;
        }

        QSize   dims;

#if KDCRAW_VERSION < 0x000400
        QString rawFilesExt(KDcrawIface::DcrawBinary::instance()->rawFiles());
#else
        QString rawFilesExt(KDcrawIface::KDcraw::rawFiles());
#endif
        QString ext = fileInfo.suffix().toUpper();

        if (!ext.isEmpty() && rawFilesExt.toUpper().contains(ext))
        {
            str = i18n("RAW Image");
            dims = metaData.getImageDimensions();
        }
        else
        {
            str = fi.mimeComment();

            KFileMetaInfo meta = fi.metaInfo();

/*          TODO: KDE4PORT: KFileMetaInfo API as Changed.
                            Check if new method to search "Dimensions" information is enough.

            if (meta.isValid())
            {
                if (meta.containsGroup("Jpeg EXIF Data"))
                    dims = meta.group("Jpeg EXIF Data").item("Dimensions").value().toSize();
                else if (meta.containsGroup("General"))
                    dims = meta.group("General").item("Dimensions").value().toSize();
                else if (meta.containsGroup("Technical"))
                    dims = meta.group("Technical").item("Dimensions").value().toSize();
            }*/

            if (meta.isValid() && meta.item("Dimensions").isValid())
            {
                dims = meta.item("Dimensions").value().toSize();
            }
        }

        if (settings.showImageType)
        {
            tipText += m_cellBeg + i18n("Type:") + m_cellMid + str + m_cellEnd;
        }

        if (settings.showImageDim)
        {
            QString mpixels;
            mpixels.setNum(dims.width()*dims.height()/1000000.0, 'f', 2);
            str = (!dims.isValid()) ? i18n("Unknown") : i18n("%1x%2 (%3Mpx)",
                    dims.width(), dims.height(), mpixels);
            tipText += m_cellBeg + i18n("Dimensions:") + m_cellMid + str + m_cellEnd;
        }
    }

    // -- Photograph Info ----------------------------------------------------

    if (settings.showPhotoMake  ||
        settings.showPhotoDate  ||
        settings.showPhotoFocal ||
        settings.showPhotoExpo  ||
        settings.showPhotoMode  ||
        settings.showPhotoFlash ||
        settings.showPhotoWB)
    {
        PhotoInfoContainer photoInfo = metaData.getPhotographInformations();

        if (!photoInfo.isEmpty())
        {
            QString metaStr;
            tipText += m_headBeg + i18n("Photograph Properties") + m_headEnd;

            if (settings.showPhotoMake)
            {
                str = QString("%1 / %2").arg(photoInfo.make.isEmpty() ? unavailable : photoInfo.make)
                                        .arg(photoInfo.model.isEmpty() ? unavailable : photoInfo.model);
                if (str.length() > m_maxStringLen) str = str.left(m_maxStringLen-3) + "...";
                metaStr += m_cellBeg + i18n("Make/Model:") + m_cellMid + Qt::escape( str ) + m_cellEnd;
            }

            if (settings.showPhotoDate)
            {
                if (photoInfo.dateTime.isValid())
                {
                    str = KGlobal::locale()->formatDateTime(photoInfo.dateTime, KLocale::ShortDate, true);
                    if (str.length() > m_maxStringLen) str = str.left(m_maxStringLen-3) + "...";
                    metaStr += m_cellBeg + i18n("Created:") + m_cellMid + Qt::escape( str ) + m_cellEnd;
                }
                else
                    metaStr += m_cellBeg + i18n("Created:") + m_cellMid + Qt::escape( unavailable ) + m_cellEnd;
            }

            if (settings.showPhotoFocal)
            {
                str = photoInfo.aperture.isEmpty() ? unavailable : photoInfo.aperture;

                if (photoInfo.focalLength35mm.isEmpty())
                    str += QString(" / %1").arg(photoInfo.focalLength.isEmpty() ? unavailable : photoInfo.focalLength);
                else
                    str += QString(" / %1").arg(i18n("%1 (35mm: %2)",
                           photoInfo.focalLength, photoInfo.focalLength35mm));

                if (str.length() > m_maxStringLen) str = str.left(m_maxStringLen-3) + "...";
                metaStr += m_cellBeg + i18n("Aperture/Focal:") + m_cellMid + Qt::escape( str ) + m_cellEnd;
            }

            if (settings.showPhotoExpo)
            {
                str = QString("%1 / %2").arg(photoInfo.exposureTime.isEmpty() ? unavailable :
                                             photoInfo.exposureTime)
                                        .arg(photoInfo.sensitivity.isEmpty() ? unavailable :
                                             i18n("%1 ISO", photoInfo.sensitivity));
                if (str.length() > m_maxStringLen) str = str.left(m_maxStringLen-3) + "...";
                metaStr += m_cellBeg + i18n("Exposure/Sensitivity:") + m_cellMid + Qt::escape( str ) + m_cellEnd;
            }

            if (settings.showPhotoMode)
            {

                if (photoInfo.exposureMode.isEmpty() && photoInfo.exposureProgram.isEmpty())
                    str = unavailable;
                else if (!photoInfo.exposureMode.isEmpty() && photoInfo.exposureProgram.isEmpty())
                    str = photoInfo.exposureMode;
                else if (photoInfo.exposureMode.isEmpty() && !photoInfo.exposureProgram.isEmpty())
                    str = photoInfo.exposureProgram;
                else
                    str = QString("%1 / %2").arg(photoInfo.exposureMode).arg(photoInfo.exposureProgram);
                if (str.length() > m_maxStringLen) str = str.left(m_maxStringLen-3) + "...";
                metaStr += m_cellBeg + i18n("Mode/Program:") + m_cellMid + Qt::escape( str ) + m_cellEnd;
            }

            if (settings.showPhotoFlash)
            {
                str = photoInfo.flash.isEmpty() ? unavailable : photoInfo.flash;
                if (str.length() > m_maxStringLen) str = str.left(m_maxStringLen-3) + "...";
                metaStr += m_cellBeg + i18n("Flash:") + m_cellMid + Qt::escape( str ) + m_cellEnd;
            }

            if (settings.showPhotoWB)
            {
                str = photoInfo.whiteBalance.isEmpty() ? unavailable : photoInfo.whiteBalance;
                if (str.length() > m_maxStringLen) str = str.left(m_maxStringLen-3) + "...";
                metaStr += m_cellBeg + i18n("White Balance:") + m_cellMid + Qt::escape( str ) + m_cellEnd;
            }

            tipText += metaStr;
        }
    }

    return tipText;
}

QString ThumbBarToolTip::breakString(const QString& input) const
{
    QString str = input.simplified();
    str = Qt::escape(str);
    const int maxLen = m_maxStringLen;

    if (str.length() <= maxLen)
        return str;

    QString br;

    int i = 0;
    int count = 0;

    while (i < str.length())
    {
        if (count >= maxLen && str[i].isSpace())
        {
            count = 0;
            br.append("<br/>");
        }
        else
        {
            br.append(str[i]);
        }

        i++;
        count++;
    }

    return br;
}

}  // namespace Digikam
