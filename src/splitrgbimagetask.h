#ifndef SPLITRGBIMAGETASK_H
#define SPLITRGBIMAGETASK_H

#include <QObject>
#include <QImage>

QImage splitRGBImage(const QImage &inputImage);

class SplitRGBImageTask : public QObject
{
    Q_OBJECT
public:
    SplitRGBImageTask(QObject *parent = nullptr);
    void setImage(QImage inputImage);
public slots:
    void run();
signals:
    void done(QImage RGBImage);
    void workFinished();
private:
    QImage image;
};

#endif // SPLITRGBIMAGETASK_H
