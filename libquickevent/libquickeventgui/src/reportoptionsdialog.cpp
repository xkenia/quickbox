#include "reportoptionsdialog.h"
#include "ui_reportoptionsdialog.h"

#include <qf/qmlwidgets/framework/mainwindow.h>
#include <qf/qmlwidgets/framework/plugin.h>

#include <qf/core/string.h>
#include <qf/core/assert.h>

#include <QSettings>
#include <QShowEvent>
#include <QSqlDatabase>
#include <QTimer>

namespace quickevent {
namespace gui {

namespace {
auto persistent_settings_path_prefix = QStringLiteral("ui/MainWindow/");
auto default_persistent_settings_id =  QStringLiteral("reportOptionsDialog");
}

ReportOptionsDialog::ReportOptionsDialog(QWidget *parent)
	: QDialog(parent)
	, qf::qmlwidgets::framework::IPersistentSettings(this)
	, ui(new Ui::ReportOptionsDialog)
{
	ui->setupUi(this);
	setPersistentSettingsId(default_persistent_settings_id);

	//ui->edFilter->setText("h1%");
	ui->grpStartOptions->setVisible(false);
	ui->grpStartersOptions->setVisible(false);
	ui->grpStages->setVisible(false);
	ui->grpLegs->setVisible(false);
	ui->grpResultOptions->setVisible(false);
	ui->grpStartTimes->setVisible(false);
	ui->btRegExp->setEnabled(QSqlDatabase::database().driverName().endsWith(QLatin1String("PSQL"), Qt::CaseInsensitive));

	connect(ui->btSaveAsDefault, &QPushButton::clicked, [this]() {
		savePersistentSettings();
	});
	connect(this, &ReportOptionsDialog::persistentSettingsIdChanged, [this]() {
		loadPersistentSettings();
	});

	connect(this, &ReportOptionsDialog::startListOptionsVisibleChanged, ui->grpStartOptions, &QGroupBox::setVisible);
	connect(this, &ReportOptionsDialog::classFilterVisibleChanged, ui->grpClassFilter, &QGroupBox::setVisible);
	connect(this, &ReportOptionsDialog::startersOptionsVisibleChanged, ui->grpStartersOptions, &QGroupBox::setVisible);
	connect(this, &ReportOptionsDialog::vacantsVisibleChanged, ui->chkStartOpts_PrintVacants, &QCheckBox::setVisible);
	connect(this, &ReportOptionsDialog::stagesOptionVisibleChanged, ui->grpStages, &QGroupBox::setVisible);
	connect(this, &ReportOptionsDialog::legsOptionVisibleChanged, ui->grpLegs, &QGroupBox::setVisible);
	connect(this, &ReportOptionsDialog::pageLayoutVisibleChanged, ui->grpPageLayout, &QGroupBox::setVisible);
	connect(this, &ReportOptionsDialog::columnCountEnableChanged, ui->edColumnCount, &QGroupBox::setEnabled);
	connect(this, &ReportOptionsDialog::resultOptionsVisibleChanged, ui->grpResultOptions, &QGroupBox::setVisible);
	connect(this, &ReportOptionsDialog::startTimeFormatVisibleChanged, ui->grpStartTimes, &QGroupBox::setVisible);

	//connect(ui->edStagesCount, &QSpinBox::valueChanged, [this](int n) {
	//	qfInfo() << "stage cnt value changed:" << n;
	//});
}

ReportOptionsDialog::~ReportOptionsDialog()
{
	delete ui;
}

QString ReportOptionsDialog::persistentSettingsPath()
{
	return persistent_settings_path_prefix + persistentSettingsId();
}

bool ReportOptionsDialog::setPersistentSettingsId(const QString &id)
{
	bool ret = qf::qmlwidgets::framework::IPersistentSettings::setPersistentSettingsId(id);
	if(ret)
		emit persistentSettingsIdChanged(id);
	return ret;
}

void ReportOptionsDialog::setClassNamesFilter(const QStringList &class_names)
{
	ui->grpClassFilter->setChecked(true);
	ui->btClassNames->setChecked(true);
	ui->chkClassFilterDoesntMatch->setChecked(false);
	ui->edFilter->setText(class_names.join(','));
}

int ReportOptionsDialog::stagesCount() const
{
	return ui->edStagesCount->value();
}

void ReportOptionsDialog::setStagesCount(int n)
{
	ui->edStagesCount->setValue(n);
}

bool ReportOptionsDialog::resultExcludeDisq() const
{
	return ui->chkExcludeDisq->isChecked();
}

void ReportOptionsDialog::setResultExcludeDisq(bool b)
{
	ui->chkExcludeDisq->setChecked(b);
}

ReportOptionsDialog::BreakType ReportOptionsDialog::breakType() const
{
	return static_cast<BreakType>(ui->cbxBreakAfterClassType->currentIndex());
}

bool ReportOptionsDialog::isStartListPrintVacants() const
{
	return ui->chkStartOpts_PrintVacants->isChecked();
}

bool ReportOptionsDialog::isStartListPrintStartNumbers() const
{
	return ui->chkStartOpts_PrintStartNumbers->isChecked();
}

QString ReportOptionsDialog::sqlWhereExpression() const
{
	const Options opts = options();
	return sqlWhereExpression(opts);
}

QString ReportOptionsDialog::sqlWhereExpression(const ReportOptionsDialog::Options &opts)
{
	if(opts.isUseClassFilter()) {
		QString filter_str = opts.classFilter();
		if(!filter_str.isEmpty()) {
			FilterType filter_type = (FilterType)opts.classFilterType();
			if(filter_type == FilterType::RegExp) {
				QString filter_operator = opts.isInvertClassFilter()? "!~*": "~*";
				QString ret = "classes.name %1 '%2'";
				ret = ret.arg(filter_operator).arg(filter_str);
				return ret;
			}
			else if(filter_type == FilterType::WildCard) {
				filter_str.replace('*', '%').replace('?', '_');
				QString filter_operator = opts.isInvertClassFilter()? "NOT LIKE": "LIKE";
				QString ret = "classes.name %1 '%2'";
				ret = ret.arg(filter_operator).arg(filter_str);
				return ret;
			}
			else if(filter_type == FilterType::ClassName) {
				qf::core::String s = filter_str;
				QStringList sl = s.splitAndTrim(',');
				QString filter_operator = opts.isInvertClassFilter()? "NOT IN": "IN";
				QString ret = "classes.name %1('%2')";
				ret = ret.arg(filter_operator).arg(sl.join("','"));
				return ret;
			}
		}
	}
	return QString();
}
/*
void ReportOptionsDialog::showEvent(QShowEvent *event)
{
	Super::showEvent(event);
	if(event->spontaneous())
		return;
	ui->grpClassFilter->setVisible(isClassFilterVisible());
}
*/

int ReportOptionsDialog::exec()
{
	//ui->grpClassFilter->setVisible(isClassFilterVisible());
	return Super::exec();
}

void ReportOptionsDialog::setStartListPrintVacantsVisible(bool b)
{
	if(!b)
		ui->chkStartOpts_PrintVacants->setChecked(b);
	ui->chkStartOpts_PrintVacants->setVisible(b);
}

void ReportOptionsDialog::setOptions(const ReportOptionsDialog::Options &options)
{
	//qfLogFuncFrame() << options;
	ui->edStagesCount->setValue(options.stagesCount());
	ui->edLegsCount->setValue(options.legsCount());
	//qfInfo() << "options.stagesCount()" << options.stagesCount() << ui->edStagesCount->value();
	ui->cbxBreakAfterClassType->setCurrentIndex(options.breakType());
	ui->edColumnCount->setValue(options.columns().length() / 2 + 1);
	ui->edPageWidth->setValue(options.pageWidth());
	ui->edPageHeight->setValue(options.pageHeight());
	ui->edHorizontalMargin->setValue(options.horizontalMargin());
	ui->edVerticalMargin->setValue(options.verticalMargin());
	//ui->chkShirinkPageWidthToColumnCount->setChecked(options.isShirinkPageWidthToColumnCount());
	ui->grpClassFilter->setChecked(options.isUseClassFilter());
	ui->chkClassFilterDoesntMatch->setChecked(options.isInvertClassFilter());
	ui->edFilter->setText(options.classFilter());
	FilterType filter_type = (FilterType)options.classFilterType();
	ui->btWildCard->setChecked(filter_type == FilterType::WildCard);
	ui->btRegExp->setChecked(filter_type == FilterType::RegExp);
	ui->btClassNames->setChecked(filter_type == FilterType::ClassName);
	ui->chkStartOpts_PrintVacants->setChecked(options.isStartListPrintVacants());
	ui->chkStartOpts_PrintStartNumbers->setChecked(options.isStartListPrintStartNumbers());
	ui->edStartersOptionsLineSpacing->setValue(options.startersOptionsLineSpacing());
	ui->edNumPlaces->setValue(options.resultNumPlaces());
	ui->chkExcludeDisq->setChecked(options.isResultExcludeDisq());
	StartTimeFormat start_time_format = (StartTimeFormat)options.startTimeFormat();
	ui->btStartTimes1->setChecked(start_time_format == StartTimeFormat::RelativeToClassStart);
	ui->btStartTimes2->setChecked(start_time_format == StartTimeFormat::DayTime);
}

ReportOptionsDialog::Options ReportOptionsDialog::options() const
{
	Options opts;
	opts.setStagesCount(ui->edStagesCount->value());
	opts.setLegsCount(ui->edLegsCount->value());
	opts.setBreakType(ui->cbxBreakAfterClassType->currentIndex());
	opts.setColumnCount(ui->edColumnCount->value());
	QString columns;
	for (int i = 0; i < ui->edColumnCount->value(); ++i)
		columns += i>0? ",%": "%";
	opts.setColumns(columns);
	opts.setPageWidth(ui->edPageWidth->value());
	opts.setPageHeight(ui->edPageHeight->value());
	opts.setHorizontalMargin(ui->edHorizontalMargin->value());
	opts.setVerticalMargin(ui->edVerticalMargin->value());
	//opts.setShirinkPageWidthToColumnCount(ui->chkShirinkPageWidthToColumnCount->isChecked());
	opts.setUseClassFilter(ui->grpClassFilter->isChecked());
	opts.setInvertClassFilter(ui->chkClassFilterDoesntMatch->isChecked());
	opts.setClassFilter(ui->edFilter->text());
	FilterType filter_type =  ui->btWildCard->isChecked()? FilterType::WildCard: ui->btRegExp->isChecked()? FilterType::RegExp: FilterType::ClassName;
	opts.setClassFilterType((int)filter_type);
	opts.setStartListPrintVacants(isStartListPrintVacants());
	opts.setStartListPrintStartNumbers(isStartListPrintStartNumbers());
	opts.setStartersOptionsLineSpacing(ui->edStartersOptionsLineSpacing->value());
	opts[QStringLiteral("isBreakAfterEachClass")] = isBreakAfterEachClass();
	opts[QStringLiteral("isColumnBreak")] = isColumnBreak();
	opts.setResultNumPlaces(ui->edNumPlaces->value());
	opts.setResultExcludeDisq(ui->chkExcludeDisq->isChecked());
	StartTimeFormat start_time_format =  ui->btStartTimes1->isChecked()? StartTimeFormat::RelativeToClassStart: StartTimeFormat::DayTime;
	opts.setStartTimeFormat((int)start_time_format);
	return opts;
}

ReportOptionsDialog::Options ReportOptionsDialog::savedOptions(const QString &persistent_settings_id)
{
	QSettings settings;
	QString id = persistent_settings_id;
	if(id.isEmpty())
		id = default_persistent_settings_id;
	QVariantMap m = settings.value(persistent_settings_path_prefix + id).toMap();
	return Options(m);
}

QVariantMap ReportOptionsDialog::reportProperties() const
{
	Options opts = options();
	QVariantMap props;
	props["isBreakAfterEachClass"] = (opts.breakType() != (int)BreakType::None);
	props["isColumnBreak"] = (opts.breakType() == (int)BreakType::Column);
	props["options"] = opts;
	return props;
}

void ReportOptionsDialog::loadPersistentSettings(const ReportOptionsDialog::Options &default_options)
{
	qfLogFuncFrame() << persistentSettingsPath();
	if(persistentSettingsId().isEmpty())
		return;
	QSettings settings;
	QVariantMap m = settings.value(persistentSettingsPath()).toMap();
	//qfInfo() << persistentSettingsPath() << m;
	Options opts(m);
	QMapIterator<QString, QVariant> it(default_options);
	while(it.hasNext()) {
		it.next();
		if(!opts.contains(it.key()))
			opts[it.key()] = it.value();
	}
	//qfDebug() << opts;
	setOptions(opts);
}

void ReportOptionsDialog::loadPersistentSettings()
{
	loadPersistentSettings(Options());
}

void ReportOptionsDialog::savePersistentSettings()
{
	qfLogFuncFrame() << persistentSettingsPath();
	if(persistentSettingsId().isEmpty())
		return;

	Options opts = options();
	qfInfo() << persistentSettingsPath();// << opts;
	QSettings settings;
	settings.setValue(persistentSettingsPath(), opts);
}

int ReportOptionsDialog::resultNumPlaces() const
{
	return ui->edNumPlaces->value();
}

ReportOptionsDialog::StartTimeFormat ReportOptionsDialog::startTimeFormat() const
{
	return ui->btStartTimes1->isChecked()? StartTimeFormat::RelativeToClassStart: StartTimeFormat::DayTime;
}


}}
