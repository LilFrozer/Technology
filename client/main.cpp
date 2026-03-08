#include "client.h"

#include <QApplication>
#include <memory>

int main(int argc, char *argv[])
{
    try {
        QApplication a(argc, argv);

        Client::get_instance().show();

        return a.exec();
    } catch (const std::exception &e) {
        qDebug() << e.what();
    }
}
