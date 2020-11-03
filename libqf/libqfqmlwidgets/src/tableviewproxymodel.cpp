#include "tableviewproxymodel.h"

#include <qf/core/log.h>
#include <qf/core/collator.h>
#include <qf/core/model/tablemodel.h>

#include <QColor>
#include <QTextCodec>
#include <QDateTime>

//namespace qfm = qf::core::model;
using namespace qf::qmlwidgets;

TableViewProxyModel::TableViewProxyModel(QObject *parent)
	: Super(parent)
{
	setSortRole(qf::core::model::TableModel::SortRole);
}

TableViewProxyModel::~TableViewProxyModel()
{

}

void TableViewProxyModel::setRowFilterString(const QString &s)
{
	qfLogFuncFrame() << s;
	QByteArray ba = qf::core::Collator::toAscii7(QLocale::Czech, s, true);
	qfDebug() << ba;
	if(ba == m_rowFilterString)
		return;
	m_rowFilterString = ba;
	invalidateFilter();
}

QString TableViewProxyModel::rowFilterString() const
{
	return m_rowFilterString;
}

bool TableViewProxyModel::isIdle() const
{
	return m_rowFilterString.isEmpty() && sortColumn() < 0;
}

void TableViewProxyModel::sort()
{
	if(m_sortColumns.isEmpty())
		return;
	Super::sort(m_sortColumns.first(), m_sortOrder);
}

void TableViewProxyModel::sort(int column, Qt::SortOrder order)
{
	m_sortColumns.clear();
	m_sortColumns << column;
	m_sortOrder = order;
	Super::sort(column, m_sortOrder);
}

void TableViewProxyModel::addSortColumn(int column)
{
	if(!m_sortColumns.contains(column)) {
		m_sortColumns << column;
		//qfInfo() << "sort columns:" << m_sortColumns;
		Super::sort(column, m_sortOrder);
	}
}

QVariant TableViewProxyModel::data(const QModelIndex &index, int role) const
{
	QVariant ret = Super::data(index, role);
	if(!m_rowFilterString.isEmpty()) {
		if(role == Qt::BackgroundRole) {
			if(dataMatchFilter(data(index)))
				ret = QColor(Qt::yellow);
		}
	}
	return ret;
}

QVariant TableViewProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Vertical && role == Qt::DisplayRole)
		return section + 1;
	return Super::headerData(section, orientation, role);
}

bool TableViewProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	if(m_rowFilterString.isEmpty())
		return true;
	QAbstractItemModel *sm = sourceModel();
	for(int i=0; i<sm->columnCount(); i++) {
		QModelIndex ix = sm->index(source_row, i, source_parent);
		if(dataMatchFilter(sm->data(ix)))
			return true;
	}
	return false;
}

int TableViewProxyModel::variantCmp(const QVariant &left, const QVariant &right) const
{
	if(left.userType() == qMetaTypeId<QString>() && right.userType() == qMetaTypeId<QString>()) {
		const QByteArray lb = qf::core::Collator::toAscii7(QLocale::Czech, left.toString(), true);
		const QByteArray rb = qf::core::Collator::toAscii7(QLocale::Czech, right.toString(), true);
		int lsz = lb.size();
		int rsz = rb.size();
		int i;
		for(i=0; i<lsz && i<rsz; i++) {
			char lc = lb.at(i);
			char rc = rb.at(i);
			if(lc == rc)
				continue;
			if (lc < rc)
				return -1;
			return 1;
		}
		if(lsz == rsz)
			return 0;
		if(lsz < rsz)
			return -1;
		return 1;
	}
	if(!left.isValid() && !right.isValid())
		return 0;
	if(!left.isValid() && right.isValid())
		return -1;
	if(left.isValid() && !right.isValid())
		return 1;
	bool left_null = left.isNull() || (left.userType() == QVariant::String && left == QStringLiteral("null"));
	bool right_null = right.isNull() || (right.userType() == QVariant::String && right == QStringLiteral("null"));
	if(left_null && right_null)
		return 0;
	if(left_null && !right_null)
		return -1;
	if(!left_null && right_null)
		return 1;
#ifdef VARIANT_CMP
	// QVariant::operator<() deprecated since 5.15.0
	if (left < right)
		return -1;
	return (left > right)? 1: 0;
#else
	switch (left.userType()) {
	case QVariant::Bool: {
		bool l = left.toBool();
		bool r = right.toBool();
		if(l < r)
			return -1;
		if(l > r)
			return 1;
		return 0;
	}
	case QVariant::Int:
		return left.toInt() - right.toInt();
	case QVariant::UInt:
		return left.toUInt() - right.toUInt();
	case QVariant::LongLong:
		return left.toLongLong() - right.toLongLong();
	case QVariant::ULongLong:
		return left.toULongLong() - right.toULongLong();
	case QMetaType::Float:
		return left.toFloat() - right.toFloat();
	case QVariant::Double:
		return left.toDouble() - right.toDouble();
	case QVariant::Char:
		return left.toChar().unicode() - right.toChar().unicode();
	case QVariant::Date:
		return left.toDate().toJulianDay() - right.toDate().toJulianDay();
	case QVariant::Time:
		return left.toTime().msecsSinceStartOfDay() - right.toTime().msecsSinceStartOfDay();
	case QVariant::DateTime:
		return left.toDateTime().toMSecsSinceEpoch() - right.toDateTime().toMSecsSinceEpoch();
	default:
		auto ret = left.toString().compare(right.toString());
		qfWarning() << left << left.toString() << "vs" << right << right.toString() << "==" << ret;
		return ret;
	}
#endif
}

bool TableViewProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	//qfInfo() << "rows:" << left.row() << right.row();
	bool ret = false;
	const QAbstractItemModel *source_model = sourceModel();
	if(source_model) {
		for (int i = 0; i < m_sortColumns.count(); i++) {
			int column = m_sortColumns[i];
			QVariant lv = source_model->data(left.sibling(left.row(), column), sortRole()); /// comparing display role is not working for NULL values
			QVariant rv = source_model->data(right.sibling(right.row(), column), sortRole());
			int cmp = variantCmp(lv, rv);
			//qfInfo() << "\tcol:" << column << lv.toString() << "vs" << rv.toString() << "->" << cmp;
			if(cmp == 0)
				continue;
			ret = (cmp < 0);
			break;
		}
	}
	//qfInfo() << "ret:" << ret;
	return ret;
}

bool TableViewProxyModel::dataMatchFilter(const QVariant &d) const
{
	QString s = d.toString();
	QByteArray ba = qf::core::Collator::toAscii7(QLocale::Czech, s, true);
	bool ret = ba.contains(m_rowFilterString);
	return ret;
}

