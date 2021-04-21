#include "imageopstask.h"

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

/** rgb2lab function from colorspace.c
 * @url https://getreuer.info/posts/colorspace/index.html
 * @author Pascal Getreuer 2005-2010 <getreuer@gmail.com>
 **/

/** @brief XYZ color of the D65 white point */
#define WHITEPOINT_X	0.950456
#define WHITEPOINT_Y	1.0
#define WHITEPOINT_Z	1.088754

/**
 * @brief sRGB gamma correction, transforms R to R'
 * http://en.wikipedia.org/wiki/SRGB
 */
#define GAMMACORRECTION(t)	\
    (((t) <= 0.0031306684425005883) ? \
    (12.92*(t)) : (1.055*pow((t), 0.416666666666666667) - 0.055))

/**
 * @brief Inverse sRGB gamma correction, transforms R' to R
 */
#define INVGAMMACORRECTION(t)	\
    (((t) <= 0.0404482362771076) ? \
    ((t)/12.92) : pow(((t) + 0.055)/1.055, 2.4))

/**
 * @brief CIE L*a*b* f function (used to convert XYZ to L*a*b*)
 * http://en.wikipedia.org/wiki/Lab_color_space
 */
#define LABF(t)	\
    ((t >= 8.85645167903563082e-3) ? \
    pow(t,0.333333333333333) : (841.0/108.0)*(t) + (4.0/29.0))

/**
 * @brief CIE L*a*b* inverse f function
 * http://en.wikipedia.org/wiki/Lab_color_space
 */
#define LABINVF(t)	\
    ((t >= 0.206896551724137931) ? \
    ((t)*(t)*(t)) : (108.0/841.0)*((t) - (4.0/29.0)))

void rgb2xyz(double *X, double *Y, double *Z, double R, double G, double B)
{
    R = INVGAMMACORRECTION(R);
    G = INVGAMMACORRECTION(G);
    B = INVGAMMACORRECTION(B);
    *X = (double)(0.4123955889674142161*R + 0.3575834307637148171*G + 0.1804926473817015735*B);
    *Y = (double)(0.2125862307855955516*R + 0.7151703037034108499*G + 0.07220049864333622685*B);
    *Z = (double)(0.01929721549174694484*R + 0.1191838645808485318*G + 0.9504971251315797660*B);
}

void xyz2lab(double *L, double *a, double *b, double X, double Y, double Z)
{
    X /= WHITEPOINT_X;
    Y /= WHITEPOINT_Y;
    Z /= WHITEPOINT_Z;
    X = LABF(X);
    Y = LABF(Y);
    Z = LABF(Z);
    *L = 116*Y - 16;
    *a = 500*(X - Y);
    *b = 200*(Y - Z);
}

void rgb2lab(double *L, double *a, double *b, double R, double G, double B)
{
    double X, Y, Z;
    rgb2xyz(&X, &Y, &Z, R, G, B);
    xyz2lab(L, a, b, X, Y, Z);
}

#define MATH_CAST_8U(t)  (unsigned char)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)
static inline int math_round (double x)
{
    return _mm_cvtsd_si32(_mm_set_sd(x));
}

QImage splitLabImageTask(const QImage &inputImage)
{
    if (!inputImage.isGrayscale()) {
        double L, a, b;
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
                    rgb2lab(&L, &a, &b,
                            src_ptr[idx]/255.0, src_ptr[idx + 1]/255.0, src_ptr[idx + 2]/255.0);
                    dst_ptr[x] = MATH_CAST_8U(math_round(L * 255 / 100));
                    dst_ptr[x + offset_x] = MATH_CAST_8U(math_round(a + 128));
                    dst_ptr[x + offset_x + offset_x] = MATH_CAST_8U(math_round(b + 128));
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
                    rgb2lab(&L, &a, &b,
                            src_ptr[idx]/255.0, src_ptr[idx + 1]/255.0, src_ptr[idx + 2]/255.0);
                    dst_chn1_ptr[x] = MATH_CAST_8U(math_round(L * 255 / 100));
                    dst_chn2_ptr[x] = MATH_CAST_8U(math_round(a + 128));
                    dst_chn3_ptr[x] = MATH_CAST_8U(math_round(b + 128));
                }
            }
            return buf;
        }
    } else {
        return inputImage;
    }
}

SplitLabImageTask::SplitLabImageTask(QObject *parent)
    :QObject(parent)
{

}

void SplitLabImageTask::setImage(QImage inputImage)
{
    image = inputImage;
}

void SplitLabImageTask::run()
{
    QImage buf = splitLabImageTask(image);
    emit resultReady(QPixmap::fromImage(buf));
    emit workFinished();
}
