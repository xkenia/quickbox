#include "mainwindow.h"
#include "application.h"
#include "appversion.h"
#include "appclioptions.h"

#include <Core/coreplugin.h>

#include <quickevent/core/si/siid.h>
#include <quickevent/core/og/timems.h>

#include <qf/core/log.h>
#include <qf/core/logentrymap.h>
#include <qf/core/utils/settings.h>
#include <qf/core/model/logtablemodel.h>

#include <QtQml>
#include <QLocale>

#include <iostream>

namespace {

NecroLog::MessageHandler old_message_handler;
bool send_log_entry_recursion_lock = false;

void send_log_entry_handler(NecroLog::Level level, const NecroLog::LogContext &context, const std::string &msg)
{
	if(!send_log_entry_recursion_lock) {
		send_log_entry_recursion_lock = true;
		Application *app = Application::instance();
		if(app) {
			qf::core::LogEntryMap le;
			le.setLevel(level);
			le.setCategory(context.topic());
			le.setFile(context.file());
			le.setLine(context.line());
			le.setMessage(QString::fromStdString(msg));
			app->emitNewLogEntry(le);
		}
		send_log_entry_recursion_lock = false;
	}
	old_message_handler(level, context, msg);
}

}

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("quickbox");
	QCoreApplication::setOrganizationDomain("quickbox.org");
	QCoreApplication::setApplicationName("quickevent");
	QCoreApplication::setApplicationVersion(APP_VERSION);

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);
	QStringList args;
	for(const auto &s : shv_args)
		args << QString::fromStdString(s);

	QSettings::setDefaultFormat(QSettings::IniFormat);

	//qfError() << "QFLog(ERROR) test OK.";// << QVariant::typeToName(QVariant::Int) << QVariant::typeToName(QVariant::String);
	//qfWarning() << "QFLog(WARNING) test OK.";
	//qfInfo() << "QFLog(INFO) test OK.";
	//qfDebug() << "QFLog(DEBUG) test OK.";

	qfInfo() << "========================================================";
	qfInfo() << QDateTime::currentDateTime().toString(Qt::ISODate) << "starting" << QCoreApplication::applicationName() << "ver:" << QCoreApplication::applicationVersion();
	qfInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	qfInfo() << "========================================================";

	qRegisterMetaType<qf::core::LogEntryMap>();
	quickevent::core::og::TimeMs::registerQVariantFunctions();
	quickevent::core::si::SiId::registerQVariantFunctions();

	AppCliOptions cli_opts;
	cli_opts.parse(args);
	cli_opts.parse(args);
	if(cli_opts.isParseError()) {
		foreach(QString err, cli_opts.parseErrors())
			qfError() << err;
		return EXIT_FAILURE;
	}
	if(cli_opts.isAppBreak()) {
		if(cli_opts.isHelp())
			cli_opts.printHelp();
		return EXIT_SUCCESS;
	}
	foreach(QString s, cli_opts.unusedArguments()) {
		qDebug() << "Undefined argument:" << s;
	}

	// Uncaught exception is intentional here
	if(!cli_opts.loadConfigFile()) {
		return EXIT_FAILURE;
	}

	qDebug() << "creating application instance";
	Application app(argc, argv, &cli_opts);

	old_message_handler = NecroLog::setMessageHandler(send_log_entry_handler);

	QString lc_name;
	{
		if(cli_opts.locale_isset()) {
			lc_name = cli_opts.locale();
		}
		else {
			qf::core::utils::Settings settings;
			lc_name = settings.value(Core::CorePlugin::SETTINGS_PREFIX_APPLICATION_LOCALE_LANGUAGE()).toString();
		}
		if(lc_name.isEmpty() || lc_name == QLatin1String("system"))
			lc_name = QLocale::system().name();
		QString app_translations_path = QCoreApplication::applicationDirPath() + "/translations";
		qfInfo() << "Loading translations for:" << lc_name;
		{
			QTranslator *qt_translator = new QTranslator(&app);
			QString tr_name = "qt_" + lc_name;
			bool ok = qt_translator->load(tr_name, app_translations_path);
			if(ok) {
				ok = app.installTranslator(qt_translator);
				qfInfo() << "Installing translator file:" << tr_name << " ... " << (ok? "OK": "ERROR");
			}
			else {
				qfInfo() << "Erorr loading translator file:" << (app_translations_path + '/' + tr_name);
			}
		}
		for(QString prefix : {"libqfcore", "libqfqmlwidgets", "libquickeventcore", "libquickeventgui", "libsiut", "quickevent"}) {
			QTranslator *qt_translator = new QTranslator(&app);
			QString tr_name = prefix + "." + lc_name;
			bool ok = qt_translator->load(tr_name, app_translations_path);
			if(ok) {
				ok = app.installTranslator(qt_translator);
				qfInfo() << "Installing translator file:" << tr_name << " ... " << (ok? "OK": "ERROR");
			}
			else {
				qfInfo() << "Erorr loading translator file:" << (app_translations_path + '/' + tr_name);
			}
		}
	}

	MainWindow main_window;
	main_window.setUiLanguageName(lc_name);
	main_window.loadPlugins();
	main_window.show();

	int ret = app.exec();

	return ret;
}
