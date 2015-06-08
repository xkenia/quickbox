#ifndef RUNSWIDGET_H
#define RUNSWIDGET_H

#include <QFrame>

namespace qf {
namespace core {
namespace model {
class SqlTableModel;
}
}
namespace qmlwidgets {
class ForeignKeyComboBox;
}
}

namespace Event {
class EventPlugin;
}

namespace Ui {
class RunsWidget;
}

class ThisPartWidget;
class RunsTableModel;
class RunsTableItemDelegate;

class RunsWidget : public QFrame
{
	Q_OBJECT
private:
	typedef QFrame Super;
public:
	explicit RunsWidget(QWidget *parent = 0);
	~RunsWidget() Q_DECL_OVERRIDE;

	void settleDownInPartWidget(ThisPartWidget *part_widget);

	static Event::EventPlugin* eventPlugin();

	Q_SLOT void reset(int class_id = 0);
	Q_SLOT void reload();

	void editStartList(int class_id, int competitor_id);
private slots:
	void on_btDraw_clicked();
	void on_btDrawRemove_clicked();

private:
	int currentStageId();

	Q_SLOT void lazyInit();

	/**
	 * @brief runnersInClubsHistogram
	 * @return list of runs.id for each club sorted by their count, longest list of runners is first
	 */
	QList< QList<int> > runnersByClubSortedByCount(int stage_id, int class_id, QMap<int, QString> &runner_id_to_club);
	QList<int> runnersForClass(int stage_id, int class_id);

	void onCustomContextMenuRequest(const QPoint &pos);
private:
	enum class DrawMethod : int {Invalid = 0, RandomNumber, EquidistantClubs, RandomizedEquidistantClubs};

	Ui::RunsWidget *ui;
	RunsTableModel *m_runsModel;
	RunsTableItemDelegate *m_runsTableItemDelegate;
	qf::qmlwidgets::ForeignKeyComboBox *m_cbxClasses = nullptr;
};

#endif // RUNSWIDGET_H
