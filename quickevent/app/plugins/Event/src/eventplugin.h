#ifndef EVENTPLUGIN_H
#define EVENTPLUGIN_H

#include "eventpluginglobal.h"
#include "eventconfig.h"
#include "stage.h"

#include <qf/core/utils.h>
#include <qf/qmlwidgets/framework/plugin.h>

#include <QVariantMap>
#include <QSqlDriver>

namespace qf { namespace core { namespace sql { class Query; class Connection; }}}
namespace qf { namespace qmlwidgets { class Action; } }
namespace qf { namespace qmlwidgets { namespace framework { class DockWidget; }}}

class QComboBox;
class DbSchema;

namespace Event {

class EVENTPLUGIN_DECL_EXPORT EventPlugin : public qf::qmlwidgets::framework::Plugin
{
	Q_OBJECT
	Q_PROPERTY(QObject* eventConfig READ eventConfig)
	Q_PROPERTY(int currentStageId READ currentStageId NOTIFY currentStageIdChanged)
	Q_PROPERTY(int stageCount READ stageCount)
	Q_PROPERTY(QString eventName READ eventName WRITE setEventName NOTIFY eventNameChanged)
	Q_PROPERTY(bool eventOpen READ isEventOpen WRITE setEventOpen NOTIFY eventOpenChanged)
	Q_PROPERTY(bool sqlServerConnected READ isSqlServerConnected NOTIFY sqlServerConnectedChanged)
private:
	using Super = qf::qmlwidgets::framework::Plugin;
public:
	enum class ConnectionType : int {SqlServer = 0, SingleFile};
public:
	EventPlugin(QObject *parent = nullptr);

	QF_PROPERTY_BOOL_IMPL(e, E, ventOpen)
	QF_PROPERTY_IMPL(QString, e, E, ventName)

	static const char *DBEVENT_NOTIFY_NAME;
	static const char* DBEVENT_COMPETITOR_COUNTS_CHANGED; //< number of competitors in classes changed
	static const char* DBEVENT_CARD_READ;
	//static const char* DBEVENT_CARD_CHECKED;
	static const char* DBEVENT_CARD_PROCESSED_AND_ASSIGNED;
	static const char* DBEVENT_PUNCH_RECEIVED;
	static const char* DBEVENT_REGISTRATIONS_IMPORTED;
	static const char* DBEVENT_STAGE_START_CHANGED;

	Q_INVOKABLE void initEventConfig();
	Event::EventConfig* eventConfig(bool reload = false);
	int stageCount();

	Q_SLOT void setCurrentStageId(int stage_id);
	int currentStageId();
	Q_SIGNAL void currentStageIdChanged(int current_stage);

	Q_INVOKABLE int stageIdForRun(int run_id);

	Q_INVOKABLE int stageStartMsec(int stage_id);
	Q_INVOKABLE QDate stageStartDate(int stage_id);
	Q_INVOKABLE QTime stageStartTime(int stage_id);
	Q_INVOKABLE QDateTime stageStartDateTime(int stage_id);
	//Q_INVOKABLE int currentStageStartMsec();
	int msecToStageStartAM(int si_am_time_sec, int msec = 0, int stage_id = 0);

	//Q_INVOKABLE QVariantMap stageDataMap(int stage_id) {return stageData(stage_id);}
	void setStageData(int stage_id, const QString &key, const QVariant &value);
	StageData stageData(int stage_id);
	Q_SLOT void clearStageDataCache();

	Q_SLOT bool createEvent(const QString &event_name = QString(), const QVariantMap &event_params = QVariantMap());
	Q_SLOT void editEvent();
	Q_SLOT bool closeEvent();
	Q_SLOT bool openEvent(const QString &event_name = QString());
	Q_SLOT void exportEvent_qbe();
	Q_SLOT void importEvent_qbe();

	Q_SIGNAL void reloadDataRequest();

	bool isSqlServerConnected() const { return m_sqlServerConnected; }
	Q_SIGNAL void sqlServerConnectedChanged(bool is_open);

	Q_INVOKABLE void emitDbEvent(const QString &domain, const QVariant &data = QVariant(), bool loopback = true);
	Q_SIGNAL void dbEventNotify(const QString &domain, int connection_id, const QVariant &payload);

	Q_INVOKABLE QString sqlDriverName();

	Q_INVOKABLE QString classNameById(int class_id);

	DbSchema* dbSchema();
	static int minDbVersion();

	Q_SLOT void onInstalled();
public:
	// event wide signals
	//Q_SIGNAL void editStartListRequest(int stage_id, int class_id, int competitor_id);
private:
	void setSqlServerConnected(bool ok);

	ConnectionType connectionType() const;
	QStringList existingSqlEventNames() const;
	QStringList existingFileEventNames(const QString &dir = QString()) const;

	Q_SLOT void onEventOpened();
	Q_SLOT void connectToSqlServer();
	Q_SLOT void onCbxStageActivated(int ix);
	Q_SLOT void loadCurrentStageId();
	Q_SLOT void saveCurrentStageId(int current_stage);
	Q_SLOT void editStage();
	Q_SLOT void onDbEvent(const QString & name, QSqlDriver::NotificationSource source, const QVariant & payload);

	void onDbEventNotify(const QString &domain, int connection_id, const QVariant &data);

	void updateWindowTitle();

	//bool runSqlScript(qf::core::sql::Query &q, const QStringList &sql_lines);
	void repairStageStarts(const qf::core::sql::Connection &from_conn, const qf::core::sql::Connection &to_conn);

	void onServiceDockVisibleChanged(bool on = true);
private:
	qf::qmlwidgets::Action *m_actConnectDb = nullptr;
	qf::qmlwidgets::Action *m_actEvent = nullptr;
	qf::qmlwidgets::Action *m_actImport = nullptr;
	qf::qmlwidgets::Action *m_actExport = nullptr;
	qf::qmlwidgets::Action *m_actCreateEvent = nullptr;
	qf::qmlwidgets::Action *m_actOpenEvent = nullptr;
	qf::qmlwidgets::Action *m_actEditEvent = nullptr;
	qf::qmlwidgets::Action *m_actExportEvent_qbe = nullptr;
	qf::qmlwidgets::Action *m_actImportEvent_qbe = nullptr;
	qf::qmlwidgets::Action *m_actEditStage = nullptr;
	Event::EventConfig *m_eventConfig = nullptr;
	bool m_sqlServerConnected = false;
	QComboBox *m_cbxStage = nullptr;
	QMap<int, StageData> m_stageCache;
	QMap<int, QString> m_classNameCache;

	qf::qmlwidgets::framework::DockWidget *m_servicesDockWidget = nullptr;
};

}

#endif // EVENTPLUGIN_H
