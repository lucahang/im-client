#include <QApplication>
#include <memory>
#include "ui/login_dialog.h"
#include "ui/mainwindow.h"
#include "imclientwrapper.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // if (argc < 3) {
    //     qCritical() << "Usage: " << argv[0] << " <host> <port>";
    //     return 1;
    // }
    // std::string host = argv[1];
    // uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    auto wrapper = std::make_shared<IMClientWrapper>("127.0.0.1", 9000);

    LoginDialog login(wrapper);
    if (login.exec() == QDialog::Accepted) {
        MainWindow mainWindow(wrapper);
        mainWindow.show();
        return app.exec();
    } else {
        return 0;
    }
}