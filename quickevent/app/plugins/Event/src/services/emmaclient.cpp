#include "emmaclient.h"
#include "emmaclientwidget.h"

#include "../eventplugin.h"

#include <quickevent/core/si/checkedcard.h>

#include <qf/qmlwidgets/framework/mainwindow.h>
#include <qf/qmlwidgets/dialogs/dialog.h>

#include <qf/core/log.h>
#include <qf/core/assert.h>
#include <qf/core/sql/querybuilder.h>
#include <qf/core/sql/query.h>
#include <qf/core/sql/connection.h>

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

namespace qfc = qf::core;
namespace qfw = qf::qmlwidgets;
namespace qfd = qf::qmlwidgets::dialogs;
namespace qfs = qf::core::sql;

namespace Event {
namespace services {

EmmaClient::EmmaClient(QObject *parent)
	: Super(EmmaClient::serviceName(), parent)
{
	connect(eventPlugin(), &Event::EventPlugin::dbEventNotify, this, &EmmaClient::onDbEventNotify, Qt::QueuedConnection);

	m_exportTimer = new QTimer(this);
	connect(m_exportTimer, &QTimer::timeout, this, &EmmaClient::onExportTimerTimeOut);
	connect(this, &EmmaClient::statusChanged, [this](Status status) {
		if(status == Status::Running) {
			if(settings().exportIntervalSec() > 0) {
				onExportTimerTimeOut();
				m_exportTimer->start();
			}
		}
		else {
			m_exportTimer->stop();
		}
	});
	connect(this, &EmmaClient::settingsChanged, this, &EmmaClient::init, Qt::QueuedConnection);

}

QString EmmaClient::serviceName()
{
	return QStringLiteral("EmmaClient");
}

void EmmaClient::exportRadioCodes()
{
	EmmaClientSettings ss = settings();
	QString export_dir = ss.exportDir();
	QDir ed;
	if(!createExportDir()) {
		return;
	}
	QString event_name = eventPlugin()->eventName();
	QFile f_splitnames(export_dir + '/' + event_name + ".splitnames.txt");
	if(!f_splitnames.open(QFile::WriteOnly)) {
		qfError() << "Canot open file:" << f_splitnames.fileName() << "for writing.";
		return;
	}
	qfInfo() << "EmmaClient: exporting code names to" << f_splitnames.fileName();
	QFile f_splitcodes(ss.exportDir() + '/' + event_name + ".splitcodes.txt");
	if(!f_splitcodes.open(QFile::WriteOnly)) {
		qfError() << "Canot open file:" << f_splitcodes.fileName() << "for writing.";
		return;
	}
	qfInfo() << "EmmaClient: exporting codes to" << f_splitcodes.fileName();

	QTextStream ts_names(&f_splitnames);
	QTextStream ts_codes(&f_splitcodes);

	int current_stage = eventPlugin()->currentStageId();
	qfs::QueryBuilder qb_classes;
	qb_classes.select2("classes", "name")
			.select2("classdefs", "courseId")
			.from("classes")
			.joinRestricted("classes.id", "classdefs.classId", "classdefs.stageId=" + QString::number(current_stage))
			.orderBy("classes.name");
	qfs::Query q1;
	q1.execThrow(qb_classes.toString());
	while(q1.next()) {
		int course_id = q1.value("courseId").toInt();
		qfs::QueryBuilder qb_codes;
		qb_codes.select2("codes", "*")
				//.select2("coursecodes", "position")
				.from("coursecodes")
				.joinRestricted("coursecodes.codeId", "codes.id", "codes.radio", qfs::QueryBuilder::INNER_JOIN)
				.where("coursecodes.courseId=" + QString::number(course_id))
				.orderBy("coursecodes.position");
		//qfInfo() << qb_codes.toString();

		QString class_name = q1.value("classes.name").toString();
        class_name.remove(" ");
		QVector<int> codes;
		qfs::Query q2;
		q2.execThrow(qb_codes.toString());
		while(q2.next()) {
			int code = q2.value("codes.code").toInt();
			codes << code;
		}

		ts_names << class_name;
		ts_codes << class_name;
		if(!codes.isEmpty()) {
			for(int code : codes) {
				ts_names << ' ' << QStringLiteral("cn%1").arg(code);
				ts_codes << ' ' << code;
			}
		}
		ts_names << " finish\n";
		ts_codes << ' ' << 2 << '\n';
	}
}

void EmmaClient::exportResultsIofXml3()
{
	if(!createExportDir())
		return;
	QString event_name = eventPlugin()->eventName();
	EmmaClientSettings ss = settings();
	QString export_dir = ss.exportDir();
	QString file_name = export_dir + '/' + event_name + ".results.xml";
	int current_stage = eventPlugin()->currentStageId();
	qf::qmlwidgets::framework::MainWindow *fwk = qf::qmlwidgets::framework::MainWindow::frameWork();
	QObject *plugin = fwk->plugin("Runs");
	QMetaObject::invokeMethod(plugin, "exportResultsIofXml30Stage",
							  Q_ARG(int, current_stage),
							  Q_ARG(QString, file_name) );
}

bool EmmaClient::createExportDir()
{
	EmmaClientSettings ss = settings();
	QString export_dir = ss.exportDir();
	QDir ed;
	if(!ed.mkpath(export_dir)) {
		qfError() << "Canot create export dir:" << export_dir;
		return false;
	}
	return true;
}

void EmmaClient::onDbEventNotify(const QString &domain, int connection_id, const QVariant &data)
{
	Q_UNUSED(connection_id)
	Q_UNUSED(data)
	//qfInfo() << domain << data;
	if(domain == QLatin1String(Event::EventPlugin::DBEVENT_CARD_PROCESSED_AND_ASSIGNED)) {
		onCardChecked(data.toMap());
	}
}

void EmmaClient::onCardChecked(const QVariantMap &data)
{
	Q_UNUSED(data)
/*  regenerate file every X second
	if(status() != Status::Running)
		return;
	quickevent::core::si::CheckedCard checked_card(data);
	QString s = QString("%1").arg(checked_card.cardNumber(), 8, 10, QChar(' '));
	s += QStringLiteral(": FIN/");
	int64_t msec = checked_card.stageStartTimeMs() + checked_card.finishTimeMs();
	QTime tm = QTime::fromMSecsSinceStartOfDay(msec);
	s += tm.toString(QStringLiteral("HH:mm:ss.zzz"));
	s += '0';
	s += '/';
	if (checked_card.finishTimeMs() > 0) {
		if (checked_card.isMisPunch() || checked_card.isBadCheck()) {
			s += QStringLiteral("MP  ");
		} else {
			//checked_card is OK
			s += QStringLiteral("O.K.");
		}
	} else {
		// DidNotFinish
		s += QStringLiteral("DNF ");
	}
	qfInfo() << "EmmaClient: " << s;
	EmmaClientSettings ss = settings();
	QString fn = ss.exportDir() + '/' + ss.fileName();
	QFile file(fn);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
		qfError() << "Cannot open file" << file.fileName() << "for writing, stopping service";
		stop();
		return;
	}
	QTextStream out(&file);
	out << s << "\n";*/
}

qf::qmlwidgets::framework::DialogWidget *EmmaClient::createDetailWidget()
{
	auto *w = new EmmaClientWidget();
	return w;
}

void EmmaClient::init()
{
	EmmaClientSettings ss = settings();
	if(ss.exportIntervalSec() > 0) {
		m_exportTimer->setInterval(ss.exportIntervalSec() * 1000);
	}
	else {
		m_exportTimer->stop();
	}
}

bool EmmaClient::preExport()
{
	EmmaClientSettings ss = settings();
	if(!QDir().mkpath(ss.exportDir())) {
		qfError() << "Cannot create export dir:" << ss.exportDir();
		return false;
	}
	return true;
}

void EmmaClient::onExportTimerTimeOut()
{
	if(status() != Status::Running)
		return;

	if (!preExport())
		return;

	EmmaClientSettings ss = settings();

	if (ss.exportTypeXML3())
	{
		qfInfo() << "EmmaClient Iof Xml3 creation called";
		exportResultsIofXml3();
	}
	if (ss.exportStart())
	{
		qfInfo() << "EmmaClient startlist creation called";
		exportStartList();
	}
	if (ss.exportFinish())
	{
		qfInfo() << "EmmaClient finish creation called";
		exportFinish();
	}
}

void EmmaClient::exportFinish()
{
	EmmaClientSettings ss = settings();
	QString export_dir = ss.exportDir();
	QFile f(export_dir + '/' + ss.fileName() + ".finish.txt");
	if(!f.open(QFile::WriteOnly)) {
		qfError() << "Canot open file:" << f.fileName() << "for writing.";
		return;
	}

	QTextStream ts(&f);

	int current_stage = eventPlugin()->currentStageId();
	qfs::QueryBuilder qb;
	qb.select2("cards", "id, siId")
            .select2("runs", "finishTimeMs, misPunch, badCheck, disqualified, notCompeting")
			.from("cards")
			.join("cards.runId", "runs.id")
			.where("cards.stageId=" QF_IARG(current_stage))
            .orderBy("cards.id ASC");
	int start00 = eventPlugin()->stageStartMsec(current_stage);

	qfs::Query q2;
	q2.execThrow(qb.toString());
	while(q2.next()) {
		int si = q2.value("cards.siId").toInt();
		int finTime = q2.value("runs.finishTimeMs").toInt();
		bool isMisPunch = q2.value("runs.misPunch").toBool();
		bool isBadCheck = q2.value("runs.badCheck").toBool();
		bool isDisq = q2.value("runs.disqualified").toBool();
        bool notCompeting = q2.value("runs.notCompeting").toBool();
		QString s = QString("%1").arg(si , 8, 10, QChar(' '));
		s += QStringLiteral(": FIN/");
		int msec = start00 + finTime;
		QTime tm = QTime::fromMSecsSinceStartOfDay(msec);
		s += tm.toString(QStringLiteral("HH:mm:ss.zzz"));
		s += '0';
		s += '/';
		if (finTime > 0) {
            if (notCompeting) {
                s += QStringLiteral("NC  ");
            }
            else if (isMisPunch || isBadCheck) {
				s += QStringLiteral("MP  ");
			} else if (isDisq) {
				s += QStringLiteral("DISQ");
            } else {
				//checked_card is OK
				s += QStringLiteral("O.K.");
			}
		} else {
			// DidNotFinish
			s += QStringLiteral("DNF ");
		}

		ts << s << "\n";
	}
}

void EmmaClient::exportStartList()
{
	EmmaClientSettings ss = settings();
	QString export_dir = ss.exportDir();
	QFile f(export_dir + '/' + ss.fileName() + ".start.txt");
	if(!f.open(QFile::WriteOnly)) {
		qfError() << "Canot open file:" << f.fileName() << "for writing.";
		return;
	}

	QTextStream ts(&f);
	ts.setGenerateByteOrderMark(true); // BOM

	bool is_relays = eventPlugin()->eventConfig()->isRelays();
	int current_stage = eventPlugin()->currentStageId();
	qfs::QueryBuilder qb;
	qb.select2("runs", "startTimeMs, siId, competitorId, isrunning, leg")
            .select2("competitors","firstName, lastName, registration")
            .select2("classes","name")
            .select2("cards", "id, siId, startTime")
			.from("runs")
			.join("runs.competitorId","competitors.id")
            .join("runs.id", "cards.runId")
			.where("runs.stageId=" QF_IARG(current_stage));
	if(is_relays) {
		qb.select2("relays","number");
		qb.join("runs.relayId", "relays.id");
		qb.join("relays.classId", "classes.id");
		qb.orderBy("runs.leg, relays.number ASC");
	}
	else {
		qb.join("competitors.classId","classes.id");
		qb.orderBy("runs.id ASC");
	}

	int start00 = eventPlugin()->stageStartMsec(current_stage);
	qfDebug() << qb.toString();
	qfs::Query q2;
	q2.execThrow(qb.toString());
    int lastId = -1;
	QMap <int,int> startTimesRelays;
	while(q2.next()) {
        int id = q2.value("runs.competitorId").toInt();
        if (id == lastId)
            continue;
        bool isRunning = (q2.value("runs.isrunning").isNull()) ? false : q2.value("runs.isrunning").toBool();
        if (!isRunning)
            continue;
        lastId = id;
        int si = q2.value("runs.siId").toInt();
        int siCards = q2.value("cards.siId").toInt();
        int startTime = q2.value("runs.startTimeMs").toInt();
        int startTimeCard = q2.value("cards.startTime").toInt();
        if (startTimeCard == 61166)
            startTimeCard = 0;
        QString name = q2.value("competitors.lastName").toString() + " " + q2.value("competitors.firstName").toString();
        QString clas = q2.value("classes.name").toString();
        clas.remove(" ");
        QString reg = q2.value("competitors.registration").toString();
        name = name.leftJustified(22,QChar(' '),true);
		if (is_relays)
		{
			int leg = q2.value("runs.leg").toInt();
			clas = QString("%1 %2").arg(clas).arg(leg);
			clas = clas.leftJustified(7,QChar(' '),true);
			reg = reg.left(3);
			reg = reg.leftJustified(7,QChar(' '),true);
			// EmmaClient uses for all relays start time of 1st leg
			int rel_num =  q2.value("relays.number").toInt();
			if (leg == 1 && rel_num != 0) {
				startTimesRelays.insert(rel_num,startTime);
			} else if (rel_num != 0) {
				auto it = startTimesRelays.find(rel_num);
				if (it != startTimesRelays.end())
					startTime = it.value();
			}
		}
		else
		{
			clas = clas.leftJustified(7,QChar(' '),true);
			reg = reg.leftJustified(7,QChar(' '),true);
		}

        int msec = startTime;
		if (startTimeCard != 0)
        {
            // has start in si card (P, T, HDR)
            startTimeCard *= 1000; // msec
            startTimeCard -= start00;
            if (startTimeCard < 0) // 12h format
                startTimeCard += (12*60*60*1000);
            msec = startTimeCard;
        }

        QString tm2;
        //TODO zmenit na format mmm.ss,zzzz
        if (msec < 0)
            continue; // emma client has problem with negative times
        int min = (msec / 60000);
        if(min < 10)
            tm2 += "00";
        else if(min < 100)
            tm2 += "0";
        tm2 += QString::number(min);
        tm2 += '.';
        int sec = (msec % 60000 / 1000);
        if(sec < 10)
            tm2 += "0";
        tm2 += QString::number(sec);
        tm2 += ',';
        int zzzz = msec % 1000 * 10;
        if(zzzz < 10)
            tm2 += "000";
        else if(zzzz < 100)
            tm2 += "00";
        else if(zzzz < 1000)
            tm2 += "000";
        tm2 += QString::number(zzzz);

        if (siCards != 0 && siCards != si)
            si = siCards;

        if (id != 0) // filter bad data
        {
			QString s = QString("%1 %2 %3 %4 %5 %6").arg(id , 5, 10, QChar(' ')).arg(si, 8, 10, QChar(' ')).arg(clas).arg(reg).arg(name).arg(tm2);
            ts << s << "\n";
        }
	}
}

void EmmaClient::loadSettings()
{
	Super::loadSettings();
	init();
}

}}
