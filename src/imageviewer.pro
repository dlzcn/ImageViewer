QT += widgets
requires(qtConfig(filedialog))
qtHaveModule(printsupport): QT += printsupport

HEADERS       = imageviewer.h \
    QImageViewer.h \
    busyappfilter.h \
    splitrgbimagetask.h
SOURCES       = imageviewer.cpp \
                QImageViewer.cpp \
                busyappfilter.cpp \
                main.cpp \
                splitrgbimagetask.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/imageviewer
INSTALLS += target

win32: RC_ICONS = icon.ico
win32:VERSION = 1.5.0.12 # major.minor.patch.build
else:VERSION = 1.5.0    # major.minor.patch
