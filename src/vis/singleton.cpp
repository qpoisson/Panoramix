#include "qt_glue.hpp"
#include "qt_opengl_object.hpp"
#include "singleton.hpp"

namespace panoramix {
    namespace vis {

        static char * appName = "Qt Gui";

        QApplication* Singleton::InitGui(int argc, char ** argv){
            if (qApp)
                return qApp;
            QApplication* app = new QApplication(argc, argv);
            app->setQuitOnLastWindowClosed(true);
            return app;
        }

        QApplication* Singleton::InitGui(){
            return InitGui(1, &appName);
        }

        void Singleton::ContinueGui(){
            if (!qApp){
                qDebug() << "call InitGui first!";
                return;
            }
            qApp->setQuitOnLastWindowClosed(true);            
            qApp->exec();
        }


    }
}