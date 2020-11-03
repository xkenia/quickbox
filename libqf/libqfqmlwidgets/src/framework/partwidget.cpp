#include "partwidget.h"
#include "plugin.h"
#include "mainwindow.h"
#include "../frame.h"
#include "../menubar.h"
#include "../toolbar.h"
#include "../dialogs/internal/captionframe.h"

#include <qf/core/log.h>
#include <qf/core/utils.h>
#include <qf/core/assert.h>

#include <QVBoxLayout>
#include <QIcon>

using namespace qf::qmlwidgets::framework;

PartWidget::PartWidget(const QString &feature_id, QWidget *parent)
	: Super(parent)
	, IPersistentSettings(this)
{
	qfLogFuncFrame();
	setFeatureId(feature_id);
	m_captionFrame = new qf::qmlwidgets::dialogs::internal::CaptionFrame(this);
	m_centralFrame = new Frame(this);
	m_centralFrame->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	m_centralFrame->setLayoutType(Frame::LayoutVertical);
	QBoxLayout *ly = new QVBoxLayout(this);
	ly->setMargin(0);
	ly->setSpacing(1);
	ly->addWidget(m_captionFrame);
	qfDebug() << "\t adding:" << m_centralFrame << "to layout:" << ly;
	ly->addWidget(m_centralFrame);
	setLayout(ly);
}

PartWidget::~PartWidget()
{
	qfLogFuncFrame() << this << "title:" << title();
	//savePersistentSettingsRecursively();
	qDeleteAll(m_attachedObjects);
	m_attachedObjects.clear();
}

void PartWidget::setTitle(QString arg)
{
	if (m_title != arg) {
		m_title = arg;
		emit titleChanged(arg);
	}
}

void PartWidget::setIconSource(QString arg)
{
	if (m_iconSource != arg) {
		m_iconSource = arg;
		emit iconSourceChanged(arg);
	}
}

void PartWidget::setFeatureId(QString id)
{
	if (m_featureId != id) {
		m_featureId = id;
		//setProperty("persistentSettingsId", id);
		setPersistentSettingsId(id);
		//setObjectName(id);
		emit featureIdChanged(id);
	}
}

void PartWidget::updateCaptionFrame()
{
	m_captionFrame->setText(m_title);
	//m_captionFrame->setIconSource(this->iconSource());
	//QString feature_id = featureId();
	QIcon ico = createIcon();
	m_captionFrame->setIcon(ico);
}

qf::qmlwidgets::MenuBar *PartWidget::menuBar()
{
	if(!m_menuBar) {
		m_menuBar = new MenuBar();
		QBoxLayout *ly = qobject_cast<QBoxLayout*>(layout());
		QF_ASSERT_EX(ly != nullptr, "wrong layout");
		ly->insertWidget(1, m_menuBar);
	}
	return m_menuBar;
}

qf::qmlwidgets::ToolBar *PartWidget::toolBar(const QString &name, bool create_if_not_exists)
{
	qf::qmlwidgets::ToolBar *ret = m_toolBars.value(name);
	if(ret) {
		return ret;
	}
	if(!create_if_not_exists) {
		return nullptr;
	}
	if(m_toolBarsLayout == nullptr) {
		QFrame *frm = new QFrame(this);
		QBoxLayout *ly = qobject_cast<QBoxLayout*>(layout());
		QF_ASSERT_EX(ly != nullptr, "wrong layout");
		int ix = (m_menuBar)? 2: 1;
		ly->insertWidget(ix, frm);

		m_toolBarsLayout = new QHBoxLayout(frm);
		m_toolBarsLayout->setMargin(0);
		m_toolBarsLayout->setSpacing(1);
		m_toolBarsLayout->addStretch();
	}
	ret = new qf::qmlwidgets::ToolBar(this);
	m_toolBarsLayout->insertWidget(m_toolBars.count(), ret);
	m_toolBars[name] = ret;
	return ret;
}

bool PartWidget::isAddToPartSwitchFromBottom()
{
	bool add_from_bottom = false;
	auto framework = framework::MainWindow::frameWork();
	if(framework) {
		auto plugin = framework->plugin(featureId());
		if(plugin) {
			auto manifest = plugin->manifest();
			if(manifest)
				add_from_bottom = manifest->isAddFromBottom();
		}
	}
	return add_from_bottom;
}

QIcon PartWidget::createIcon()
{
	qfLogFuncFrame() << this << "feature:" << featureId();
	QIcon ico;
	QString feature_id = featureId();
	if(feature_id.isEmpty()) {
		qfWarning() << "featureId property of part widget is empty, default icon will be set.";
	}
	else {
		Plugin *plugin = MainWindow::frameWork()->plugin(feature_id);
		if(!plugin) {
			qfWarning() << "Cannot found plugin for part featureId:" << feature_id << ", default icon will be set.";
		}
		else {
			QString icon_path = iconSource();
			if(icon_path.isEmpty())
				icon_path = "images/feature";
			if(!icon_path.startsWith(":/")) {
				PluginManifest *manifest = plugin->manifest();
				if(manifest)
					icon_path = plugin->manifest()->homeDir() + "/" + icon_path;
			}
			qfDebug() << plugin << "feature:" << plugin->manifest()->featureId() << "icon path:" << icon_path;
			ico = QIcon(icon_path);
			if(ico.isNull())
				qfWarning() << "Cannot load icon on path:" << icon_path;
		}
	}
	if(ico.isNull())
		ico = QIcon(":/qf/qmlwidgets/images/under-construction.png");
	return ico;
}

QQmlListProperty<QObject> PartWidget::attachedObjects()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	return QQmlListProperty<QObject>(this, m_attachedObjects);
#else
	return QQmlListProperty<QObject>(this, &m_attachedObjects);
#endif
}

/*
bool PartWidget::canActivate(bool active_on)
{
	qfLogFuncFrame() << this << active_on;
	Q_UNUSED(active_on);
	return true;
}
*/
QQmlListProperty<QWidget> PartWidget::widgets()
{
	return m_centralFrame->widgets();
}

qf::qmlwidgets::Frame *PartWidget::centralFrame()
{
	QF_ASSERT_EX(m_centralFrame != nullptr, "Centralframe is NULL");
	return m_centralFrame;
}

void PartWidget::loadPersistentSettings()
{
	qfLogFuncFrame();
	QString path = persistentSettingsPath();
	qfDebug() << "\t" << this << path;
	/*
	if(!path.isEmpty()) {
		QSettings settings;
		settings.beginGroup(path);
		QRect geometry = settings.value("geometry").toRect();
		if(geometry.isValid()) {
			this->setGeometry(geometry);
		}
	}
	*/
}

void PartWidget::savePersistentSettings()
{
}

void PartWidget::classBegin()
{
}

void PartWidget::componentComplete()
{
}

qf::qmlwidgets::framework::Plugin *qf::qmlwidgets::framework::PartWidget::plugin(bool throw_exc)
{
	qf::qmlwidgets::framework::Plugin *ret = qf::qmlwidgets::framework::MainWindow::frameWork()->plugin(featureId(), throw_exc);
	return ret;
}
