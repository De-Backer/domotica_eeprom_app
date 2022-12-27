
#include <QByteArray>

#include "frame.h"

data_frame::data_frame(): QCanBusFrame(QCanBusFrame::InvalidFrame)
{
}

frame::data_frame(const QCanBusFrame &frame)
    : QCanBusFrame (frame.frameId(),frame.payload())
{
}
