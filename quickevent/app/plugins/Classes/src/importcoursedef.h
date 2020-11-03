#pragma once

#include <qf/core/utils.h>

struct ImportCourseDef : public QVariantMap
{
	QF_VARIANTMAP_FIELD(QString, n, setN, ame)
	QF_VARIANTMAP_FIELD(int, l, setL, enght)
	QF_VARIANTMAP_FIELD(int, c, setC, limb)
	QF_VARIANTMAP_FIELD(QStringList, c, setC, lasses)
	QF_VARIANTMAP_FIELD(QVariantList, c, setC, odes)
public:
	ImportCourseDef() : QVariantMap() {}
	ImportCourseDef(const QVariantMap &m) : QVariantMap(m) {}

	void addClass(const QString &class_name);
	QString toString() const;
};

