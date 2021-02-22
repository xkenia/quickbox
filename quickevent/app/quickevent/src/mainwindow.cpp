#include "mainwindow.h"
#include "application.h"
#include "loggerwidget.h"

#include <Core/coreplugin.h>
#include <Event/eventplugin.h>
#include <Classes/classesplugin.h>
#include <Competitors/competitorsplugin.h>
#include <Runs/runsplugin.h>
#include <Oris/orisplugin.h>
#include <CardReader/cardreaderplugin.h>
#include <Receipts/receiptsplugin.h>
#include <Relays/relaysplugin.h>
#include <Speaker/speakerplugin.h>

#include <qf/qmlwidgets/framework/stackedcentralwidget.h>
#include <qf/qmlwidgets/framework/partswitch.h>
#include <qf/qmlwidgets/framework/dockwidget.h>
#include <qf/qmlwidgets/menubar.h>
#include <qf/qmlwidgets/action.h>

#include <QLabel>

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) :
	Super(parent, flags)
{
	setWindowTitle(tr("Quick Event ver. %1").arg(versionString()));
	setWindowIcon(QIcon(":/quickevent/images/quickevent64.png"));
}

MainWindow::~MainWindow()
{
}

QString MainWindow::versionString() const
{
	return Application::instance()->versionString();
}

QString MainWindow::dbVersionString() const
{
	return Application::instance()->dbVersionString();
}

int MainWindow::dbVersion() const
{
	return Application::instance()->dbVersion();
}
/*
QString MainWindow::settingsPrefix_application_locale_language()
{
	static const QString s = SETTINGS_PREFIX_APPLICATION_LOCALE_LANGUAGE;
	return s;
}
*/
void MainWindow::onPluginsLoaded()
{
	{
		auto *dw = new qf::qmlwidgets::framework::DockWidget(nullptr);
		dw->setObjectName("loggingDockWidget");
		dw->setWindowTitle(tr("Application log"));
		addDockWidget(Qt::BottomDockWidgetArea, dw);
		auto *w = new LoggerWidget();
		connect(dw, &qf::qmlwidgets::framework::DockWidget::visibilityChanged, w, &LoggerWidget::onDockWidgetVisibleChanged);
		dw->setWidget(w);
		dw->hide();
		auto *a = dw->toggleViewAction();
		//a->setCheckable(true);
		a->setShortcut(QKeySequence("ctrl+shift+L"));
		menuBar()->actionForPath("view")->addActionInto(a);
	}

	auto *w = qobject_cast<qf::qmlwidgets::framework::StackedCentralWidget*>(centralWidget());
	menuBar()->actionForPath("view/toolbar")->addActionInto(w->partSwitch()->toggleViewAction());

	centralWidget()->setActivePart("Competitors", true);
	setPersistentSettingsId("MainWindow");
	loadPersistentSettings();
}

void MainWindow::loadPlugins()
{
	{
		auto *plugin = new Core::CorePlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Event::EventPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Classes::ClassesPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Competitors::CompetitorsPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Runs::RunsPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Oris::OrisPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new CardReader::CardReaderPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Receipts::ReceiptsPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Relays::RelaysPlugin(this);
		registerPlugin(plugin);
	}
	{
		auto *plugin = new Speaker::SpeakerPlugin(this);
		registerPlugin(plugin);
	}
	Super::loadPlugins();
}
