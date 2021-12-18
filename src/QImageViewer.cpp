#include "QImageViewer.h"
#include <QOpenGLWidget>
#include <QGraphicsItem>

QImageViewer::QImageViewer(QWidget *parent, bool useGL)
    : QGraphicsView(parent)
    , best_fit_(false)
    , zoom_op_scale_(1.0)
    , drag_line_profile_(false)
    , is_bilinear_transform_(false)
    , last_pos_(4, 0)
    , pixmap_(nullptr)
    , line_(nullptr)
{
    QGraphicsScene* scene = new QGraphicsScene();
    this->setScene(scene);
    if (useGL) { // Enable OpenGL ACC
        QOpenGLWidget *gl = new QOpenGLWidget();
        QSurfaceFormat format;
        // format.setSamples(4); // MSAA
        // format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        format.setSwapInterval(0); // try to turnoff v-sync
        if (QSurfaceFormat::defaultFormat().renderableType() == QSurfaceFormat::OpenGL)
            format.setVersion(3, 3);
        gl->setFormat(format);
        setViewport(gl);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    } else {
        //setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
        setBackgroundBrush(QBrush(Qt::white, Qt::SolidPattern));
    }
    this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    this->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //this->setRenderHint(QPainter::Antialiasing);
    this->setAcceptDrops(true);
}

QImageViewer::~QImageViewer()
{
}

void QImageViewer::clear()
{
    if (line_) {
        this->scene()->removeItem((QGraphicsItem*)line_);
        delete line_;
        line_ = nullptr;
        drag_line_profile_ = false;
    }
    if (pixmap_) {
        this->scene()->removeItem((QGraphicsItem*)pixmap_);
        delete pixmap_;
        pixmap_ = nullptr;
    }
}

 QPixmap QImageViewer::grab(const QRect &rectangle)
 {
     if (pixmap_ == nullptr)
         return QPixmap();

     QRect rect;
     if (!rectangle.isValid()) {
         QRectF senceRect = pixmap_->sceneBoundingRect();
         rect = this->mapFromScene(senceRect).boundingRect();
         qDebug() << "print -> " << rect;
     }

    return QGraphicsView::grab(rect);
 }

void QImageViewer::setBilinearTransform(bool enable)
{
    is_bilinear_transform_ = enable;
    if (pixmap_) {
        if (enable)
            pixmap_->setTransformationMode(Qt::SmoothTransformation);
        else
            pixmap_->setTransformationMode(Qt::FastTransformation);
    }
}

void QImageViewer::drawDragLine(bool clear)
{
    if (clear) {
        if (line_) {
            this->scene()->removeItem((QGraphicsItem*)line_);
            delete line_;
            line_ = nullptr;
        }
        drag_line_profile_ = false;
    } else {
        if (line_) {
            line_->setLine(last_pos_[0], last_pos_[1], last_pos_[2], last_pos_[3]);
        } else {
            QPen pen(QColor(0, 255, 0));
            pen.setCapStyle(Qt::RoundCap);
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(2); pen.setCosmetic(true); // cosmetic is faster
            line_ = this->scene()->addLine(
                last_pos_[0], last_pos_[1], last_pos_[2], last_pos_[3], pen);
        }
    }
}

void QImageViewer::update(int width, int height)
{
    QPixmap map(QSize(width, height));
    if (map.isNull())
        return;

    map.fill(Qt::lightGray);

    if (pixmap_) {
        pixmap_->setPixmap(map);
    } else {
        pixmap_ = this->scene()->addPixmap(map);
        pixmap_->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    if (is_bilinear_transform_)
        pixmap_->setTransformationMode(Qt::SmoothTransformation);
    else
        pixmap_->setTransformationMode(Qt::FastTransformation);

    setSceneRect(QRectF(map.rect()));
    best_fit_ = true;

    this->update();
}

void QImageViewer::display(const QPixmap& pixmap, bool update)
{
    if (pixmap.isNull())
        return;

    map_cache_ = pixmap;

    internal_display(update);
}

void QImageViewer::display(const QImage& img, bool update)
{
    if (img.isNull())
        return;

    bool rv = map_cache_.convertFromImage(img);
    if (!rv)
        return;

    internal_display(update);
}

void QImageViewer::internal_display(bool update)
{
    QRectF mapRect = QRectF(map_cache_.rect());

    if (pixmap_) {
        pixmap_->setPixmap(map_cache_);
    } else {
        pixmap_ = this->scene()->addPixmap(map_cache_);
        pixmap_->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    if (is_bilinear_transform_)
        pixmap_->setTransformationMode(Qt::SmoothTransformation);
    else
        pixmap_->setTransformationMode(Qt::FastTransformation);

    QRectF scenceRect = this->sceneRect();
    setSceneRect(mapRect);

    if (update)
        best_fit_ = true;
    if (scenceRect != mapRect)
        this->update();
}

void QImageViewer::update()
{
    if (pixmap_ == nullptr)
        return;

    if (best_fit_) {
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    } else {
        scale(zoom_op_scale_, zoom_op_scale_);
        //auto m = transform();
        //m.scale(zoom_op_scale_, zoom_op_scale_);
        //setTransform(m);
        zoom_op_scale_ = 1.0; // reset
    }
}

void QImageViewer::fitInView(const QRectF &rect, Qt::AspectRatioMode aspectRatioMode)
{
    if (!scene() || rect.isNull())
        return;

    // Reset the view scale to 1:1.
    QRectF unity = transform().mapRect(QRectF(0, 0, 1, 1));
    if (unity.isEmpty())
        return;
    scale(1 / unity.width(), 1 / unity.height());

    // Find the ideal x / y scaling ratio to fit \a rect in the view.
    // remove wired margin
    QRectF viewRect = viewport()->rect();
    if (viewRect.isEmpty())
        return;
    QRectF sceneRect = transform().mapRect(rect);
    if (sceneRect.isEmpty())
        return;
    qreal xratio = viewRect.width() / sceneRect.width();
    qreal yratio = viewRect.height() / sceneRect.height();

    // Respect the aspect ratio mode.
    switch (aspectRatioMode) {
    case Qt::KeepAspectRatio:
        xratio = yratio = qMin(xratio, yratio);
        break;
    case Qt::KeepAspectRatioByExpanding:
        xratio = yratio = qMax(xratio, yratio);
        break;
    case Qt::IgnoreAspectRatio:
        break;
    }

    // Scale and center on the center of \a rect.
    scale(xratio, yratio);
    centerOn(rect.center());
}

void QImageViewer::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        this->drawDragLine(true);
        last_pos_ = std::vector<int>(4, 0);
    }
}

void QImageViewer::mouseMoveEvent(QMouseEvent* e)
{
    auto scene_pos = mapToScene(e->pos());
    QPoint pos(static_cast<int>(scene_pos.x()), static_cast<int>(scene_pos.y()));
    if (pixmap_) {
        if (drag_line_profile_) {
            // We didn't add the line in mousePressEvent, so this cond might be invalid
            // Q_ASSERT(line_ != nullptr);
            last_pos_[2] = pos.x();
            last_pos_[3] = pos.y();
            this->drawDragLine();
        } else if (dragMode() == QGraphicsView::NoDrag) {
            auto width = pixmap_->pixmap().width();
            auto height = pixmap_->pixmap().height();
            if (scene_pos.x() < 0 || scene_pos.y() < 0
                || scene_pos.x() >= width || scene_pos.y() >= height) {
                emit pixelValueOnCursor(-1, -1, 0, 0, 0);
            } else {
                int r, g, b;
                pixmap_->pixmap().toImage().pixelColor(pos).getRgb(&r, &g, &b);
                emit pixelValueOnCursor(pos.x(), pos.y(), r, g, b);
            }
        }
    }
    //else {
    //    emit pixelValueOnCursor(pos.x(), pos.y(), 0, 0, 0);
    //}

    QGraphicsView::mouseMoveEvent(e);
}

void QImageViewer::mousePressEvent(QMouseEvent* e)
{
    auto scene_pos = mapToScene(e->pos());
    QPoint pos(static_cast<int>(scene_pos.x()), static_cast<int>(scene_pos.y()));
    if (e->button() == Qt::LeftButton) {
        auto modifiers = qApp->keyboardModifiers();
        if (modifiers & Qt::ControlModifier) {
            last_pos_[0] = pos.x();
            last_pos_[1] = pos.y();
            drag_line_profile_ = true;
        } else {
            setDragMode(QGraphicsView::ScrollHandDrag);
        }
        emit pixelValueOnCursor(-1, -1, 0, 0, 0);
    }
    QGraphicsView::mousePressEvent(e);
}

void QImageViewer::mouseReleaseEvent(QMouseEvent* e)
{
    QGraphicsView::mouseReleaseEvent(e);

    auto scene_pos = mapToScene(e->pos());
    QPoint pos(static_cast<int>(scene_pos.x()), static_cast<int>(scene_pos.y()));
    if (e->button() == Qt::LeftButton) {
        auto modifiers = qApp->keyboardModifiers();
        if (modifiers & Qt::ControlModifier && drag_line_profile_) {
            Q_ASSERT(line_ != nullptr);
            last_pos_[2] = pos.x();
            last_pos_[3] = pos.y();
            drawDragLine();
            drag_line_profile_ = false;
            emit lineProfileReady(last_pos_[0], last_pos_[1], last_pos_[2], last_pos_[3]);
        }
        setDragMode(QGraphicsView::NoDrag);
    }
}

void QImageViewer::wheelEvent(QWheelEvent* e)
{
    int v = e->angleDelta().y() / 120;
    if (v > 0) {
        zoom_op_scale_ = 1.25;
        update();
    } else if (v < 0) {
        zoom_op_scale_ = 0.75;
        update();
    }
}

void QImageViewer::resizeEvent(QResizeEvent * /* unused */)
{
    update();
}

void QImageViewer::leaveEvent(QEvent* e)
{
    emit pixelValueOnCursor(-1, -1, 0, 0, 0);
    QGraphicsView::leaveEvent(e);
}

void QImageViewer::zoomIn()
{
    best_fit_ = false;
    zoom_op_scale_ = 2.0;
    update();
}

void QImageViewer::zoomOut()
{
    best_fit_ = false;
    zoom_op_scale_ = 0.5;
    update();
}

void QImageViewer::zoomOriginal()
{
    best_fit_ = false;
    resetTransform();
    update();
}

void QImageViewer::zoomFit()
{
    best_fit_ = true;
    update();
}

double QImageViewer::getZoomScale() const
{
    return transform().m11(); // m22() for y, m33() is the factor, not used here
}

void QImageViewer::dragEnterEvent(QDragEnterEvent * e)
{
    if (e->mimeData()->hasUrls()) {
        e->setAccepted(true);
        QGraphicsView::update();
    }
}

void QImageViewer::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->setAccepted(true);
        QGraphicsView::update();
    }
}

void QImageViewer::dropEvent(QDropEvent *e)
{
    emit filesDropped(e->mimeData()->urls());
}
