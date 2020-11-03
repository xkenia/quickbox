#include "competitordocument.h"

#include <Event/eventplugin.h>

#include <qf/qmlwidgets/framework/mainwindow.h>
#include <qf/qmlwidgets/framework/plugin.h>

#include <qf/core/sql/connection.h>
#include <qf/core/sql/query.h>
#include <qf/core/sql/transaction.h>
#include <qf/core/assert.h>

using namespace Competitors;

CompetitorDocument::CompetitorDocument(QObject *parent)
	: Super(parent)
{
	qf::core::sql::QueryBuilder qb;
	qb.select2("competitors", "*")
			.select("lastName || ' ' || firstName AS name")
			//.select2("classes", "name AS className")
			.from("competitors")
			//.join("competitors.classId", "classes.id")
			.where("competitors.id={{ID}}");
	setQueryBuilder(qb);
}

static Event::EventPlugin* eventPlugin()
{
	qf::qmlwidgets::framework::MainWindow *fwk = qf::qmlwidgets::framework::MainWindow::frameWork();
	qf::qmlwidgets::framework::Plugin *plugin = fwk->plugin("Event");
	return qobject_cast<Event::EventPlugin *>(plugin);
}

bool CompetitorDocument::saveData()
{
	qfLogFuncFrame();
	RecordEditMode old_mode = mode();
	bool siid_dirty = isDirty("competitors.siId");
	/*
	if(siid_dirty) {
		int id = siid().toInt();
		if(id == 0)
			setSiid(QVariant());
	}
	*/
	bool class_dirty = isDirty("competitors.classId");
	bool ret = Super::saveData();
	qfDebug() << "Super save data:" << ret;
	if(ret) {
		if(old_mode == DataDocument::ModeInsert) {
			// insert runs
			qfDebug() << "inserting runs";
			int competitor_id = dataId().toInt();
			auto *event_plugin = eventPlugin();
			QF_ASSERT(event_plugin != nullptr, "invalid Event plugin type", return false);

			int stage_count = event_plugin->stageCount();
			qf::core::sql::Query q(model()->connectionName());
			q.prepare("INSERT INTO runs (competitorId, stageId, siId) VALUES (:competitorId, :stageId, :siId)");
			m_lastInsertedRunsIds.clear();
			for(int i=0; i<stage_count; i++) {
				q.bindValue(":competitorId", competitor_id);
				q.bindValue(":stageId", i + 1);
				if(siid_dirty)
					q.bindValue(":siId", siid());
				q.exec(qf::core::Exception::Throw);
				m_lastInsertedRunsIds << q.lastInsertId().toInt();
			}
			eventPlugin()->emitDbEvent(Event::EventPlugin::DBEVENT_COMPETITOR_COUNTS_CHANGED);
		}
		else if(old_mode == DataDocument::ModeEdit) {
			if(siid_dirty) {
				qfDebug() << "updating SIID in run tables";
				if(siid_dirty) {
					int competitor_id = dataId().toInt();
					qf::core::sql::Query q(model()->connectionName());
					q.prepare("UPDATE runs SET siId=:siId WHERE competitorId=:competitorId", qf::core::Exception::Throw);
					q.bindValue(":competitorId", competitor_id);
					q.bindValue(":siId", siid());
					q.exec(qf::core::Exception::Throw);
				}
			}
			if(class_dirty)
				eventPlugin()->emitDbEvent(Event::EventPlugin::DBEVENT_COMPETITOR_COUNTS_CHANGED);
		}
	}
	return ret;
}

bool CompetitorDocument::dropData()
{
	bool ret = false;
	auto id = dataId();
	{
		qf::core::sql::Query q(model()->connectionName());
		q.prepare("DELETE FROM runs WHERE competitorId = :competitorId");
		q.bindValue(":competitorId", id);
		ret = q.exec();
		if(!ret)
			qfError() << q.lastError().text();
	}
	if(ret) {
		ret = Super::dropData();
		eventPlugin()->emitDbEvent(Event::EventPlugin::DBEVENT_COMPETITOR_COUNTS_CHANGED);
	}
	return ret;
}
/*
bool CompetitorDocument::isSaveSiidToRuns() const
{
	return m_saveSiidToRuns;
}

void CompetitorDocument::setSaveSiidToRuns(bool save_siid_to_runs)
{
	m_saveSiidToRuns = save_siid_to_runs;
}
*/
namespace {
const auto SIID = QStringLiteral("competitors.siId");
}
/*
void CompetitorDocument::setSiid(const QVariant &siid, bool save_siid_to_runs)
{
	setValue(SIID, siid);
	m_saveSiidToRuns = save_siid_to_runs;
}
*/
void CompetitorDocument::setSiid(const QVariant &siid)
{
	setValue(SIID, siid);
}

QVariant CompetitorDocument::siid() const
{
	return value(SIID);
}

