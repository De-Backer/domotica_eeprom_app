#ifndef FRAME_H
#define FRAME_H

#include <QByteArray>
#include <QCanBusFrame>
#include <QtEndian>
#include <QtGlobal>
#include <QVector>

class QByteArray;

class data_frame : public QCanBusFrame
{
public:
    data_frame();
    data_frame(const data_frame &data_frame) = default;
    data_frame &operator=(const data_frame &data_frame) = default;

    explicit data_frame(const QCanBusFrame &data_frame);

    template <class Payload>
    Payload decode() const
    {
        auto decoded{Payload{}};
        qFromLittleEndian<qint64>(payload().data(), 1, &decoded);
        return decoded;
    }

protected:
    template <class Payload>
    static QByteArray encode(Payload payload)
    {
        QByteArray result(8, 0x00);
        qToLittleEndian(payload, result.data());
        return result;
    }
};
#endif // FRAME_H
