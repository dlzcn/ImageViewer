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
