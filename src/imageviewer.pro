QT += widgets
requires(qtConfig(filedialog))
qtHaveModule(printsupport): QT += printsupport

HEADERS       = imageviewer.h \
    QImageViewer.h \
    busyappfilter.h \
    imageopstask.h
SOURCES       = imageviewer.cpp \
                QImageViewer.cpp \
                busyappfilter.cpp \
                imageopstask.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/imageviewer
INSTALLS += target

win32: RC_ICONS = icon.ico
win32:VERSION = 1.5.1.12 # major.minor.patch.build
else:VERSION = 1.5.1    # major.minor.patch
