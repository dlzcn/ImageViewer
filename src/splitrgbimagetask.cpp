#include "splitrgbimagetask.h"

QImage splitRGBImage(const QImage &inputImage)
{
    if (!inputImage.isGrayscale()) {
        QImage src = inputImage.convertToFormat(QImage::Format_RGB888);
        int width, height;
        if (inputImage.width() < 2 * inputImage.height()) {
            width = 3 * inputImage.width();
            height = inputImage.height();
            QImage buf(width, height, QImage::Format_Grayscale8);
            for (int y = 0; y < inputImage.height(); y++) {
                unsigned char* src_ptr = src.scanLine(y);
                unsigned char* dst_ptr = buf.scanLine(y);
                int offset_x = inputImage.width();
                for (int x = 0, idx = 0; x < inputImage.width(); x++, idx += 3) {
                    dst_ptr[x] = src_ptr[idx];
                    dst_ptr[x + offset_x] = src_ptr[idx + 1];
                    dst_ptr[x + offset_x + offset_x] = src_ptr[idx + 2];
                }
            }
            return buf;
        } else {
            width = inputImage.width();
            height= 3 * inputImage.height();
            QImage buf(width, height, QImage::Format_Grayscale8);
            for (int y = 0; y < inputImage.height(); y++) {
                unsigned char* src_ptr = src.scanLine(y);
                unsigned char* dst_chn1_ptr = buf.scanLine(y);
                unsigned char* dst_chn2_ptr = buf.scanLine(y + inputImage.height());
                unsigned char* dst_chn3_ptr = buf.scanLine(y + 2 * inputImage.height());
                for (int x = 0, idx = 0; x < inputImage.width(); x++, idx += 3) {
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
        emit done(buf);
        emit workFinished();
}

