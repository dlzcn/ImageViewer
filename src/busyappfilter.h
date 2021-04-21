#ifndef BUSYAPPFILTER_H
#define BUSYAPPFILTER_H

#include <QObject>
#include <QEvent>

class BusyAppFilter : public QObject
{
public:
    BusyAppFilter(QObject *parent) : QObject(parent) {};
protected:
    bool eventFilter( QObject *obj, QEvent *event );
};

#endif // BUSYAPPFILTER_H
