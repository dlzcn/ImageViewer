#ifndef IMAGEOPSTASK_H
#define IMAGEOPSTASK_H

#include <QObject>
#include <QImage>
#include <QPixmap>

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
    void resultReady(QPixmap RGBPix);
    void workFinished();
private:
    QImage image;
};

QImage splitLabImageTask(const QImage &inputImage);

class SplitLabImageTask : public QObject
{
    Q_OBJECT
public:
    SplitLabImageTask(QObject *parent = nullptr);
    void setImage(QImage inputImage);
public slots:
    void run();
signals:
    void resultReady(QPixmap RGBPix);
    void workFinished();
private:
    QImage image;
    double r, g;
};

#endif // IMAGEOPSTASK_H
