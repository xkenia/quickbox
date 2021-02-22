#include "qmllogsingleton.h"

#include <qf/core/log.h>

#include <QQmlEngine>
#include <QJSEngine>

using namespace qf::core::qml;

QmlLogSingleton::QmlLogSingleton(QObject *parent) :
    QObject(parent)
{
}

QObject *QmlLogSingleton::singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
	//Q_UNUSED(engine)
	Q_UNUSED(scriptEngine)

	QmlLogSingleton *s = new QmlLogSingleton(engine);
	return s;
}

void QmlLogSingleton::debug(const QString &msg)
{
	qfDebug() << msg;
}

void QmlLogSingleton::info(const QString &msg)
{
	qfInfo() << msg;
}

void QmlLogSingleton::warning(const QString &msg)
{
	qfWarning() << msg;
}

void QmlLogSingleton::error(const QString &msg)
{
	qfError() << msg;
}

void QmlLogSingleton::fatal(const QString &msg)
{
	qfFatal(msg);
}
