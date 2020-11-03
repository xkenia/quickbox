#include "orisplugin.h"
#include "orisimporter.h"
#include "txtimporter.h"

#include <Event/eventplugin.h>

#include <qf/qmlwidgets/framework/mainwindow.h>
#include <qf/qmlwidgets/menubar.h>
#include <qf/qmlwidgets/action.h>

#include <qf/core/log.h>

namespace qff = qf::qmlwidgets::framework;
namespace qfw = qf::qmlwidgets;

namespace Oris {

OrisPlugin::OrisPlugin(QObject *parent)
	: Super("Oris", parent)//, qf::qmlwidgets::framework::IPersistentSettings(this)
{
	//setPersistentSettingsId("Oris");
	m_orisImporter = new OrisImporter(this);
	m_txtImporter = new TxtImporter(this);

	connect(this, &OrisPlugin::installed, this, &OrisPlugin::onInstalled);
}

void OrisPlugin::onInstalled()
{
	qfLogFuncFrame();

	qff::MainWindow *fwk = qff::MainWindow::frameWork();
	auto *event_plugin = fwk->plugin<Event::EventPlugin*>();
	//console.warn("Oris installed");
	qfw::Action *act_import = fwk->menuBar()->actionForPath("file/import");
	qf::qmlwidgets::Action *act_import_oris = act_import->addMenuInto("oris", tr("&ORIS"));
	{
		qfw::Action *a = act_import_oris->addActionInto("event", tr("&Event"));
		connect(a, &qfw::Action::triggered, m_orisImporter, &OrisImporter::chooseAndImport);
	}
	{
		qfw::Action *a = act_import_oris->addActionInto("syncEntries", tr("&Sync current event entries"));
		connect(a, &qfw::Action::triggered, m_orisImporter, &OrisImporter::syncCurrentEventEntries);
		a->setEnabled(false);
		connect(event_plugin, &Event::EventPlugin::eventOpenChanged, [a](bool is_event_open) {
			a->setEnabled(is_event_open);
		});
	}
	/*
	//act_import_oris->addSeparatorInto();
	{
		qfw::Action *a = act_import_oris->addActionInto("syncRelaysEntries", tr("Sync &relays entries"));
		connect(a, &qfw::Action::triggered, m_orisImporter, &OrisImporter::syncRelaysEntries);
		a->setVisible(false);
		connect(event_plugin, &Event::EventPlugin::eventOpenChanged, [a](bool is_db_open) {
			bool is_relays = false;
			if(is_db_open) {
				is_relays = eventPlugin()->eventConfig()->isRelays();
			}
			a->setVisible(is_relays);
		});
	}
	*/
	act_import_oris->addSeparatorInto();
	{
		qfw::Action *a = act_import_oris->addActionInto("clubs", tr("&Clubs and registrations"));
		connect(a, &qfw::Action::triggered, m_orisImporter, &OrisImporter::importClubs);
		connect(a, &qfw::Action::triggered, m_orisImporter, &OrisImporter::importRegistrations);
		a->setEnabled(false);
		connect(event_plugin, &Event::EventPlugin::eventOpenChanged, [a](bool is_event_open) {
			a->setEnabled(is_event_open);
		});
	}
	qf::qmlwidgets::Action *act_import_txt = act_import->addMenuInto("text", tr("&Text file"));
	act_import_txt->setEnabled(false);
	connect(event_plugin, &Event::EventPlugin::eventOpenChanged, [act_import_txt](bool is_event_open) {
		act_import_txt->setEnabled(is_event_open);
	});
	{
		qfw::Action *a = act_import_txt->addActionInto("competitorsCSOS", tr("&Competitors CSOS"));
		connect(a, &qfw::Action::triggered, m_txtImporter, &TxtImporter::importCompetitorsCSOS);
	}
	{
		qfw::Action *a = act_import_txt->addActionInto("competitorsCSV", tr("Competitors C&SV"));
		connect(a, &qfw::Action::triggered, m_txtImporter, &TxtImporter::importCompetitorsCSV);
	}
	{
		qfw::Action *a = act_import_txt->addActionInto("competitorsRanking", tr("&Ranking CSV (ORIS format)"));
		connect(a, &qfw::Action::triggered, m_txtImporter, &TxtImporter::importRankingCsv);
	}
}

}
