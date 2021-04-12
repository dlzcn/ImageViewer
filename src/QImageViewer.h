#pragma once

#include <QtGui>
#include <QGraphicsView>


class QImageViewer : public QGraphicsView
{
    Q_OBJECT

public:
    QImageViewer(QWidget *parent, bool useGL = false);
    ~QImageViewer();

    void update(int width, int height); // set a blank image (best fit)
    void display(const QPixmap& pixmap, bool update = false);
    void display(const QImage& img, bool update = false);
    void clear();

    QPixmap grab(const QRect &rectangle = QRect(QPoint(0, 0), QSize(-1, -1)));

    void setBilinearTransform(bool enable);
    bool getBilinearTransform() const {
        return is_bilinear_transform_;
    };

    void zoomIn();
    void zoomOut();
    void zoomOriginal();
    void zoomFit();
    void resetBestFit() { best_fit_ = false; }
    bool isBestFit() const { return best_fit_; }
    double getZoomScale() const;

    void setDragLine(bool val) { drag_line_profile_ = val; }
    bool isDragLine() const { return drag_line_profile_; }
    void drawDragLine(bool clear = false); /// draw draged line 

    std::vector<int> getDragLinePos() const { return last_pos_; }
    //std::vector<double> getDragLineData(int start_x, int start_y, int end_x, int end_y);
protected:
    virtual void internal_display(bool update);
    virtual void update();
    virtual void mouseDoubleClickEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void wheelEvent(QWheelEvent* e);
    virtual void resizeEvent(QResizeEvent *e);
    virtual void leaveEvent(QEvent* e);

signals:
    void pixelValueOnCursor(int x, int y, int r, int g, int b);
    void lineProfileReady(int start_x, int start_y, int end_x, int end_y);
private:
    bool best_fit_;
    double zoom_op_scale_;
    bool drag_line_profile_;
    bool is_bilinear_transform_;
    std::vector<int> last_pos_;
    std::vector<QRectF> zoom_stack_;
    QPixmap map_cache_;
    QGraphicsPixmapItem *pixmap_;
    QGraphicsLineItem *line_;
};

