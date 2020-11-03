message(including $$PWD)

QT += core gui qml widgets sql
# plugin sometimes cannot find Qt libraries, CardReader needs QtSerialPort.so
QT += serialport

CONFIG += C++11

TEMPLATE = app

include (../../../quickbox.pri)

PROJECT_TOP_SRCDIR = $$PWD

DESTDIR = $$QF_PROJECT_TOP_BUILDDIR/bin
TARGET = quickevent

win32: LIB_DIR_NAME = bin
else:  LIB_DIR_NAME = lib

INCLUDEPATH += \
    $$QF_PROJECT_TOP_SRCDIR/libqf/libqfcore/include \
    $$QF_PROJECT_TOP_SRCDIR/libqf/libqfqmlwidgets/include \
    $$QF_PROJECT_TOP_SRCDIR/libquickevent/libquickeventcore/include \
    $$QF_PROJECT_TOP_SRCDIR/libquickevent/libquickeventgui/include \
    $$PWD/../plugins/include \

message(INCLUDEPATH: $$INCLUDEPATH)

LIBS += \
	-L$$QF_PROJECT_TOP_BUILDDIR/$$LIB_DIR_NAME \

LIBS +=      \
    -lqfcore  \
    -lqfqmlwidgets  \
    -lsiut \
    -lquickeventcore \
    -lquickeventgui \
    -lCoreQEPlugin \
    -lEventQEPlugin \
    -lClassesQEPlugin \
    -lCompetitorsQEPlugin \
    -lRunsQEPlugin \
    -lOrisQEPlugin \
    -lCardReaderQEPlugin \
    -lReceiptsQEPlugin \
    -lRelaysQEPlugin \
    -lSpeakerQEPlugin \

unix: LIBS +=  \
	-L../../../lib  \
	-Wl,-rpath,\'\$\$ORIGIN/../lib:\$\$ORIGIN/../lib/qml/quickevent\'  \

# exception backtrace support
CONFIG(debug, debug|release): unix: QMAKE_LFLAGS += -rdynamic

RESOURCES += \
    $$PWD/images/images.qrc \

RC_FILE = $$PWD/app.rc

include ($$QF_PROJECT_TOP_SRCDIR/crosscompile-support.pri)
include ($$QF_PROJECT_TOP_SRCDIR/appdatafiles.pri)

include($$PWD/src/src.pri)

OTHER_FILES += \
#    ../plugins/qml/Core/*.qml \
    ../plugins/qml/SqlDb/*.qml \
    ../plugins/qml/Event/*.qml \
    ../plugins/qml/Help/*.qml \

QML_IMPORT_PATH += \
    $$QF_PROJECT_TOP_BUILDDIR/lib/qml \
	$$QF_PROJECT_TOP_BUILDDIR/lib/qml/quickevent \


win32:CONFIG(debug, debug|release):CONFIG += console
#CONFIG += console

TRANSLATIONS += \
	$${TARGET}.cs_CZ.ts \
	$${TARGET}.fr_FR.ts \
	$${TARGET}.nb_NO.ts \
	$${TARGET}.nl_BE.ts \
	$${TARGET}.pl_PL.ts \
	$${TARGET}.ru_RU.ts \
	$${TARGET}.uk_UA.ts \
