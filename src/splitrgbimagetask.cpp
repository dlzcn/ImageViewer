#include "splitrgbimagetask.h"

QImage splitRGBImage(const QImage &inputImage)
{
    if (!inputImage.isGrayscale()) {
        QImage src = inputImage.convertToFormat(QImage::Format_RGB888);
        int width, height;
        if (src.width() < 2 * src.height()) {
            width = 3 * src.width();
            height = src.height();
            QImage buf(width, height, QImage::Format_Grayscale8);
            for (int y = 0; y < src.height(); y++) {
                unsigned char* src_ptr = src.scanLine(y);
                unsigned char* dst_ptr = buf.scanLine(y);
                int offset_x = src.width();
                for (int x = 0, idx = 0; x < src.width(); x++, idx += 3) {
                    dst_ptr[x] = src_ptr[idx];
                    dst_ptr[x + offset_x] = src_ptr[idx + 1];
                    dst_ptr[x + offset_x + offset_x] = src_ptr[idx + 2];
                }
            }
            return buf;
        } else {
            width = src.width();
            height= 3 * src.height();
            QImage buf(width, height, QImage::Format_Grayscale8);
            for (int y = 0; y < src.height(); y++) {
                unsigned char* src_ptr = src.scanLine(y);
                unsigned char* dst_chn1_ptr = buf.scanLine(y);
                unsigned char* dst_chn2_ptr = buf.scanLine(y + src.height());
                unsigned char* dst_chn3_ptr = buf.scanLine(y + 2 * src.height());
                for (int x = 0, idx = 0; x < src.width(); x++, idx += 3) {
                    dst_chn1_ptr[x] = src_ptr[idx];
                    dst_chn2_ptr[x] = src_ptr[idx + 1];
                    dst_chn3_ptr[x] = src_ptr[idx + 2];
                }
            }
            return buf;
        }
    } else {
        return inputImage;
    }
}


SplitRGBImageTask::SplitRGBImageTask(QObject *parent)
    :QObject(parent)
{

}

void SplitRGBImageTask::setImage(QImage inputImage)
{
    image = inputImage;
}

void SplitRGBImageTask::run()
{
    QImage buf = splitRGBImage(image);
    emit resultReady(QPixmap::fromImage(buf));
    emit workFinished();
}

