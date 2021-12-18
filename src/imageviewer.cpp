/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "imageviewer.h"

#include <QApplication>
#include <QClipboard>
#include <QColorSpace>
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSettings>
#include <QProgressBar>

#if defined(QT_PRINTSUPPORT_LIB)
#  include <QtPrintSupport/qtprintsupportglobal.h>

#  if QT_CONFIG(printdialog)
#    include <QPrintDialog>
#  endif
#endif

#include "imageopstask.h"

ImageViewer::ImageViewer(QWidget *parent)
   : QMainWindow(parent)
   , imageViewer(new QImageViewer(nullptr))
   , imgPixVal(nullptr)
{
    imageViewer->setFrameStyle(QFrame::NoFrame);
    setting = new QSettings(
        QSettings::NativeFormat,
        QSettings::UserScope,
        "HF_AIO", "ImageViewer",
        this);

    progressBar = new QProgressBar(this);
    progressBar->setFixedSize(200, 16);
    progressBar->setToolTip(tr("image processing is ongoing!"));
    progressBar->hide();

    imgPixVal = new QLabel(tr("X: 0\tY: 0\n"),
                           this,
                           Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    imgPixVal->setAttribute(Qt::WA_ShowWithoutActivating);
    imgPixVal->setAlignment(Qt::AlignCenter);
    imgPixVal->setFont(QFont("Arial", 10));
    imgPixVal->hide();

    setCentralWidget(imageViewer);

    createActions();

    statusBar()->insertPermanentWidget(0, progressBar);
    resize(QGuiApplication::primaryScreen()->availableSize() * 2 / 5);

    connect(imageViewer, &QImageViewer::pixelValueOnCursor,
        this, QOverload<int,int,int,int,int>::of(&ImageViewer::updatePixelValueOnCursor));
    connect(imageViewer, &QImageViewer::filesDropped,
            this, &ImageViewer::loadDroppedFiles);

    filter = new BusyAppFilter(this);
}

bool ImageViewer::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }
    filePath = fileName;
    setImage(newImage);

    setWindowFilePath(fileName);

    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
    statusBar()->showMessage(message);
    return true;
}

void ImageViewer::setImage(const QImage &newImage)
{
    image = newImage;
    if (image.colorSpace().isValid())
        image.convertToColorSpace(QColorSpace::SRgb);

    // change default behavior
    printAct->setEnabled(true);
    dispOrigAct->setChecked(true);
    fitToWindowAct->setEnabled(true);

    displayImage(true);
//    splitAct->setChecked(false);
//    convertAct->setChecked(false);
//    mergeAct->setChecked(false);
}


bool ImageViewer::saveFile(const QString &fileName)
{
    QImageWriter writer(fileName);

    if (!writer.write(image)) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot write %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    QFileInfo imgFile(fileName);
    setting->setValue("prev_img_save_dir", imgFile.dir().absolutePath());
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    statusBar()->showMessage(message);
    return true;
}


static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode, const QString& directory = "")
{
    if (directory.isEmpty()) {
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    } else {
        dialog.setDirectory(directory);
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

void ImageViewer::open()
{

    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void ImageViewer::saveAs()
{
    QString directory = setting->value("prev_img_save_dir", "").toString();
    QFileDialog dialog(this, tr("Save File As"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave, directory);

    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}
}

void ImageViewer::print()
{
    Q_ASSERT(!image.isNull());
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
    QPrintDialog dialog(&printer, this);
    if (dialog.exec()) {
        QPainter painter(&printer);
        //QPixmap pixmap = imageLabel->pixmap(Qt::ReturnByValue);
        QPixmap pixmap = imageViewer->grab();
        QRect rect = painter.viewport();
        QSize size = pixmap.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(pixmap.rect());
        painter.drawPixmap(0, 0, pixmap);
    }
#endif
}

void ImageViewer::copy()
{
#ifndef QT_NO_CLIPBOARD
    QGuiApplication::clipboard()->setImage(image);
#endif // !QT_NO_CLIPBOARD
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull())
                return image;
        }
    }
    return QImage();
}
#endif // !QT_NO_CLIPBOARD

void ImageViewer::paste()
{
#ifndef QT_NO_CLIPBOARD
    const QImage newImage = clipboardImage();
    if (newImage.isNull()) {
        statusBar()->showMessage(tr("No image in clipboard"));
    } else {
        filePath.clear();
        setImage(newImage);
        setWindowFilePath(QString());
        const QString message = tr("Obtained image from clipboard, %1x%2, Depth: %3")
            .arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        statusBar()->showMessage(message);
    }
#endif // !QT_NO_CLIPBOARD
}

void ImageViewer::zoomIn()
{
    imageViewer->zoomIn();
}

void ImageViewer::zoomOut()
{
    imageViewer->zoomOut();
}

void ImageViewer::normalSize()
{
    imageViewer->zoomOriginal();
}

void ImageViewer::fitToWindow()
{
    bool fitToWindow = fitToWindowAct->isChecked();
    if (!fitToWindow) {
        imageViewer->zoomOriginal();
    } else {
        imageViewer->zoomFit();
    }
    updateActions();
}

void ImageViewer::about()
{
    QMessageBox::about(this, tr("About Image Viewer"),
            tr("<p>The <b>Image Viewer</b> is based on Qt Image Viewer example. "
               "It uses QGraphicView instead of QLabel for better zoom in/out, "
               "fit window and pixel value on cursor support.</p>"));
}

void ImageViewer::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &ImageViewer::open);
    openAct->setShortcut(QKeySequence::Open);

    saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &ImageViewer::saveAs);
    saveAsAct->setEnabled(false);

    printAct = fileMenu->addAction(tr("&Print..."), this, &ImageViewer::print);
    printAct->setShortcut(QKeySequence::Print);
    printAct->setEnabled(false);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcut(tr("Ctrl+Q"));

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    copyAct = editMenu->addAction(tr("&Copy"), this, &ImageViewer::copy);
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);

    QAction *pasteAct = editMenu->addAction(tr("&Paste"), this, &ImageViewer::paste);
    pasteAct->setShortcut(QKeySequence::Paste);

    editMenu->addSeparator();

    dispOrigAct = editMenu->addAction(tr("Original"), this, QOverload<bool>::of(&ImageViewer::displayImage));
    dispOrigAct->setShortcut(QKeySequence::fromString("Alt+O"));
    dispOrigAct->setEnabled(false);
    dispOrigAct->setCheckable(true);

    split1Act = editMenu->addAction(tr("Spli&t RGB"), this, &ImageViewer::toggleRGBImageDisplay);
    split1Act->setShortcut(QKeySequence::fromString("Alt+R"));
    split1Act->setEnabled(false);
    split1Act->setCheckable(true);

    split2Act = editMenu->addAction(tr("Split Lab"), this, &ImageViewer::toggleLabImageDisplay);
    split2Act->setShortcut(QKeySequence::fromString("Alt+L"));
    split2Act->setEnabled(false);
    split2Act->setCheckable(true);

    convertAct = editMenu->addAction(tr("&Illuminance"), this, &ImageViewer::toggleGrayscaleImageDisplay);
    convertAct->setShortcut(QKeySequence::fromString("Alt+I"));
    convertAct->setEnabled(false);
    convertAct->setCheckable(true);

    QActionGroup *actGrp = new QActionGroup(this);
    actGrp->addAction(dispOrigAct);
    actGrp->addAction(split1Act);
    actGrp->addAction(split2Act);
    actGrp->addAction(convertAct);
    actGrp->setExclusive(true);

    editMenu->addSeparator();

    launchAct = editMenu->addAction(tr("Ope&n Containing Folder"), this, &ImageViewer::openImagePath);
    launchAct->setShortcut(QKeySequence::fromString("Ctrl+N"));

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    zoomInAct = viewMenu->addAction(tr("Zoom &In (50%)"), this, &ImageViewer::zoomIn);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);

    zoomOutAct = viewMenu->addAction(tr("Zoom &Out (50%)"), this, &ImageViewer::zoomOut);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);

    normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &ImageViewer::normalSize);
    normalSizeAct->setShortcut(tr("Ctrl+S"));
    normalSizeAct->setEnabled(false);

    viewMenu->addSeparator();

    fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &ImageViewer::fitToWindow);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));

    QAction* bilinearTransform = viewMenu->addAction(tr("&Bilinear Transform"),
                                                     this, &ImageViewer::toggleBilinearTransform);
    bilinearTransform->setEnabled(true);
    bilinearTransform->setCheckable(true);
    bilinearTransform->setShortcut(tr("Ctrl+B"));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &ImageViewer::about);
}

void ImageViewer::updateActions()
{
    saveAsAct->setEnabled(!image.isNull());
    copyAct->setEnabled(!image.isNull());
    launchAct->setEnabled(!image.isNull());
    dispOrigAct->setEnabled(!image.isNull());
    split1Act->setEnabled(!image.isNull());
    convertAct->setEnabled(!image.isNull());
    split2Act->setEnabled(!image.isNull());
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void ImageViewer::updatePixelValueOnCursor(int x, int y, int r, int g, int b)
{
    qDebug() << x << y << r << g << b;

    if (x < 0 || y < 0) {
        imgPixVal->hide();
    } else if(image.isNull()) {
        imgPixVal->hide();
    } else {
        x = x % image.width();
        y = y % image.height();
        QString strCurrentPixelValOnCursor = tr("X: %1\tY: %2\n %3,%4,%5").arg(x, 4).arg(y, 4)
            .arg(r, 3, 'f', 0, QChar('0'))
            .arg(g, 3, 'f', 0, QChar('0'))
            .arg(b, 3, 'f', 0, QChar('0'));
        imgPixVal->setText(strCurrentPixelValOnCursor);
        imgPixVal->move(QCursor::pos() + QPoint(10, 16));
        imgPixVal->show();
    }
}

void ImageViewer::openImagePath()
{
    if (!QFileInfo::exists(filePath)) {
        qDebug() << filePath << " not exists";
        return;
    }

    QDir dir = QFileInfo(filePath).dir();

    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));

    statusBar()->showMessage(tr("Open containing folder \"%1\"").arg(dir.absolutePath()));
}

void ImageViewer::displayImage(bool)
{
    if (image.isNull())
        return;

    int size = image.size().width() * image.size().height();
    bool large_image = size > setting->value("large_image_size", 8192*8192).toInt() ? true : false;

    if (large_image) {
        qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
        installEventFilter(filter);
    }

    imageViewer->display(image, true);
    fitToWindowAct->setChecked(true);
    fitToWindow();

    if (large_image) {
        removeEventFilter(filter);
        qApp->restoreOverrideCursor();
    }
}

void ImageViewer::toggleRGBImageDisplay(bool enable)
{
    if (image.isNull() || image.isGrayscale()) {
        statusBar()->showMessage(tr("Split RGB operation ignored as source image is null or grayscale image"));
        return;
    }

    int size = image.size().width() * image.size().height();
    bool large_image = size > setting->value("large_image_size", 8192*8192).toInt() ? true : false;
    QImage buf;
    if (enable) {
        if(!large_image) {
            buf = splitRGBImage(image);
            statusBar()->showMessage(tr("Split color image into R,G,B channels"));
        } else {
            QThread* thread = new QThread();
            SplitRGBImageTask* task = new SplitRGBImageTask();
            task->setImage(image);

            // move the task object to the thread BEFORE connecting any signal/slots
            task->moveToThread(thread);

            connect(thread, &QThread::started, task, &SplitRGBImageTask::run);
            connect(task, &SplitRGBImageTask::workFinished, thread, &QThread::quit);
            connect(task, &SplitRGBImageTask::resultReady, this, QOverload<const QPixmap&>::of(&ImageViewer::displayImage));

            // automatically delete thread and task object when work is done:
            connect(task, &SplitRGBImageTask::workFinished, task, &SplitRGBImageTask::deleteLater);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);

            qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
            installEventFilter(filter);

            progressBar->show();
            progressBar->setRange(0, 0);
            thread->start();
            return;
        }
    } else {
        buf = image;
        statusBar()->showMessage(tr("Display color image"));
    }

    if (large_image) {
        qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
        installEventFilter(filter);
    }

    imageViewer->display(buf, true);
    fitToWindowAct->setChecked(true);
    fitToWindow();

    if (large_image) {
        removeEventFilter(filter);
        qApp->restoreOverrideCursor();
    }
}

void ImageViewer::displayImage(const QPixmap& image_)
{
    removeEventFilter(filter);
    qApp->restoreOverrideCursor();

    progressBar->reset();
    progressBar->hide();
    statusBar()->showMessage(tr("Image operation done"));

    imageViewer->display(image_, true);
    fitToWindowAct->setChecked(true);
    fitToWindow();
}

void ImageViewer::toggleGrayscaleImageDisplay(bool enable)
{
    if (image.isNull() || image.isGrayscale()) {
        statusBar()->showMessage(tr("Convert operation ignored as source image is null or grayscale image"));
        return;
    }

    QImage buf;
    if (enable)
        buf = image.convertToFormat(QImage::Format_Grayscale8);
    else
        buf = image;

    imageViewer->display(buf, true);
    fitToWindowAct->setChecked(true);
    fitToWindow();
}

void ImageViewer::toggleLabImageDisplay(bool enable)
{
    if (image.isNull() || image.isGrayscale()) {
        statusBar()->showMessage(tr("Split Lab operation ignored as source image is null or grayscale image"));
        return;
    }

    // double r = setting->value("merge_rg_r", 0.33).toDouble();
    // double g = setting->value("merge_rg_g", 0.66).toDouble();

    int size = image.size().width() * image.size().height();
    bool large_image = size > setting->value("large_image_size", 8192*8192).toInt() ? true : false;
    QImage buf;
    if (enable) {
        if(!large_image) {
            buf = splitLabImageTask(image);
            statusBar()->showMessage(tr("Split color image into Lab space"));
        } else {
            QThread* thread = new QThread();
            SplitLabImageTask* task = new SplitLabImageTask();
            task->setImage(image);

            // move the task object to the thread BEFORE connecting any signal/slots
            task->moveToThread(thread);

            connect(thread, &QThread::started, task, &SplitLabImageTask::run);
            connect(task, &SplitLabImageTask::workFinished, thread, &QThread::quit);
            connect(task, &SplitLabImageTask::resultReady, this, QOverload<const QPixmap&>::of(&ImageViewer::displayImage));

            // automatically delete thread and task object when work is done:
            connect(task, &SplitLabImageTask::workFinished, task, &SplitLabImageTask::deleteLater);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);

            qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
            installEventFilter(filter);

            progressBar->show();
            progressBar->setRange(0, 0);
            thread->start();
            return;
        }
    } else {
        buf = image;
        statusBar()->showMessage(tr("Display color image"));
    }

    if (large_image) {
        qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
        installEventFilter(filter);
    }

    imageViewer->display(buf, true);
    fitToWindowAct->setChecked(true);
    fitToWindow();

    if (large_image) {
        removeEventFilter(filter);
        qApp->restoreOverrideCursor();
    }
}

void ImageViewer::toggleBilinearTransform(bool enable)
{
    imageViewer->setBilinearTransform(enable);
    if (enable) {
        statusBar()->showMessage(tr("Enable Bilinear Transform (smooth image)"));
    } else {
        statusBar()->showMessage(tr("Disable Bilinear Transform (nearest neighbour)"));
    }
}

void ImageViewer::loadDroppedFiles(QList<QUrl> files)
{
    if (files.empty())
        return;

    loadFile(files.front().toLocalFile());
}
