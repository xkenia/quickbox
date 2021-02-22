
//
// Author: Frantisek Vacek <fanda.vacek@volny.cz>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef QF_QMLWIDGETS_REPORTS_REPORTPAINTER_H
#define QF_QMLWIDGETS_REPORTS_REPORTPAINTER_H

#include "reportitemframe.h"
#include "../../graphics/graphics.h"

//#include "../../qmlwidgetsglobal.h"

#include <qf/core/exception.h>
#include <qf/core/utils/treeitembase.h>

#include <QObject>
#include <QPainter>
#include <QPrinter>

namespace qf {
namespace qmlwidgets {
namespace reports {

class ReportPainter;

//! Base trida objektu, ktere vzniknou prekladem reportu.
class QFQMLWIDGETS_DECL_EXPORT ReportItemMetaPaint : public qf::core::utils::TreeItemBase
{
private:
	typedef qf::core::utils::TreeItemBase Super;
public:
	ReportItemMetaPaint();
	//! parametr \a processor v konstruktoru slouzi jenom kvuli scriptovanym atributum elementu, pouzije se jen v konstruktoru, ukazatel na nej se nikde neuklada.
	ReportItemMetaPaint(ReportItemMetaPaint *parent, ReportItem *report_item);
	~ReportItemMetaPaint() Q_DECL_OVERRIDE;
public:
	enum PaintMode {PaintBorder=1, PaintFill=2, PaintAll=3};
	//! string v reportu, ktery se vymeni za celkovy pocet stranek v reportu.
	static const QString pageCountReportSubstitution;
	static const QRegExp checkReportSubstitutionRegExp;
	static const QString checkReportSubstitution;
	//static const QString checkOffReportSubstitution;
	typedef qf::qmlwidgets::graphics::Rect Rect;
	typedef qf::qmlwidgets::graphics::Size Size;
	typedef qf::qmlwidgets::graphics::Point Point;
	class LayoutSetting : public QMap<int, QVariant>
	{
	public:
		enum {
			HInset,
			VInset,
			Layout,
			Alignment,
			SuppressPrintOut, ///<  frame a jeho deti se objevi pouze v nahledu
			FillVLayoutRatio
		};
	};
public:
	//ReportItem* reportItem();
	void setInset(qreal horizontal, qreal vertical);
	qreal insetHorizontal() {return f_layoutSettings.value(LayoutSetting::HInset).toDouble();}
	qreal insetVertical() {return f_layoutSettings.value(LayoutSetting::VInset).toDouble();}
	qf::qmlwidgets::graphics::Layout layout() const {return (qf::qmlwidgets::graphics::Layout)f_layoutSettings.value(LayoutSetting::Layout, qf::qmlwidgets::graphics::LayoutVertical).toInt();}
	void setLayout(qf::qmlwidgets::graphics::Layout ly) {if(layout() != ly) f_layoutSettings[LayoutSetting::Layout] = ly;}
	Qt::Alignment alignment() const {return (Qt::Alignment)f_layoutSettings.value(LayoutSetting::Alignment, (int)(Qt::AlignLeft | Qt::AlignTop)).toInt();}
	void setAlignment(ReportItemFrame::HAlignment hal, ReportItemFrame::VAlignment val) {
		int al = hal | val;
		if((int)alignment() != al)
			f_layoutSettings[LayoutSetting::Alignment] = al;
	}
	bool isSuppressPrintOut() {return f_layoutSettings.value(LayoutSetting::SuppressPrintOut).toBool();}
	void setSuppressPrintOut(bool b) {if(isSuppressPrintOut() != b) f_layoutSettings[LayoutSetting::SuppressPrintOut] = b;}
	double fillVLayoutRatio() {return f_layoutSettings.value(LayoutSetting::FillVLayoutRatio, -1).toDouble();}
	void setFillVLayoutRatio(double d) {if(fillVLayoutRatio() != d) f_layoutSettings[LayoutSetting::FillVLayoutRatio] = d;}

	static qf::qmlwidgets::graphics::Layout orthogonalLayout(qf::qmlwidgets::graphics::Layout l) {
		if(l == qf::qmlwidgets::graphics::LayoutHorizontal) return qf::qmlwidgets::graphics::LayoutVertical;
		if(l == qf::qmlwidgets::graphics::LayoutVertical) return qf::qmlwidgets::graphics::LayoutHorizontal;
		return qf::qmlwidgets::graphics::LayoutInvalid;
	}
	qf::qmlwidgets::graphics::Layout orthogonalLayout() const {return orthogonalLayout(layout());}
public:
	void setRenderedRectRect(const QRectF &new_size) {renderedRect = new_size;}

	ReportItemMetaPaint* parent() const Q_DECL_OVERRIDE {return dynamic_cast<ReportItemMetaPaint*>(qf::core::utils::TreeItemBase::parent());}
	ReportItemMetaPaint* child(int ix) const Q_DECL_OVERRIDE;
	virtual ReportItemMetaPaint* firstChild() const
	{
		if(childrenCount())
			return child(0);
		return nullptr;
	}
	virtual ReportItemMetaPaint* lastChild() const
	{
		if(childrenCount())
			return child(childrenCount()-1);
		return nullptr;
	}

	virtual void paint(ReportPainter *painter, unsigned mode);
	void shift(const ReportItem::Point offset)
	{
		renderedRect.translate(offset);
		shiftChildren(offset);
	}
	void shiftChildren(const ReportItem::Point offset);

	void alignChildren();

	/// popis funkce popsan u atributu expandChildFrames v Report.rnc
	void expandChildFrames();
	/// rekurzivne projde vsechny deti natahovaci ve smeru vertikalnim a nastavi jim rozmer podle sve velikosti
	void expandChildVerticalSpringFrames();
	bool hasSpringChildrenFramesInVerticalLayout();

	virtual bool isPointInside(const QPointF &p) {
		return (renderedRect.left() <= p.x() && renderedRect.right() >= p.x()
		&& renderedRect.top() <= p.y() && renderedRect.bottom() >= p.y());
	}

	virtual style::CompiledTextStyle effectiveTextStyle();
	style::CompiledTextStyle textStyle() {return m_textStyle;}
	void setTextStyle(const style::CompiledTextStyle &ts) {m_textStyle = ts;}

	virtual bool isExpandable() const {return true;}

	virtual QString dump(int indent = 0);
public:
	ReportItem::Rect renderedRect; ///< rozmery v mm

	//ReportItem *f_reportItem; /// je potreba jen kvuli selekci v report editoru
	LayoutSetting f_layoutSettings;
	style::CompiledTextStyle m_textStyle;
};


class QFQMLWIDGETS_DECL_EXPORT ReportItemMetaPaintReport : public ReportItemMetaPaint
{
private:
	typedef ReportItemMetaPaint Super;
public:
	ReportItemMetaPaintReport(ReportItem *report_item);
public:
	QPageLayout::Orientation orientation;
	QSize pageSize;
};


class ReportItemMetaPaintFrame : public ReportItemMetaPaint
{
private:
	typedef ReportItemMetaPaint Super;
public:
	ReportItemMetaPaintFrame(ReportItemMetaPaint *parent, ReportItem *report_item);
	~ReportItemMetaPaintFrame() Q_DECL_OVERRIDE {}
public:
	enum LinePos {LBrd = 1, RBrd, TBrd, BBrd};
public:
	QBrush fill;
	QPen lbrd, rbrd, tbrd, bbrd;
protected:
	virtual void fillItem(QPainter *painter, bool selected = false);
	virtual void frameItem(QPainter *painter, bool selected = false);
	void drawLine(QPainter *painter, LinePos where, const QPen &pen);
public:
	void paint(ReportPainter *painter, unsigned mode = PaintAll) Q_DECL_OVERRIDE;
};

class QFQMLWIDGETS_DECL_EXPORT ReportItemMetaPaintText : public ReportItemMetaPaint
{
private:
	typedef ReportItemMetaPaint Super;
public:
	QString text;
	QFont font;
	QPen pen; ///< barva vyplne pismen
	//bool renderCheck;
	//QBrush brush;
	QString sqlId;
	QTextOption textOption;
	QString editGrants;
public:
	void paint(ReportPainter *painter, unsigned mode = PaintAll) Q_DECL_OVERRIDE;
	bool isPointInside(const QPointF &p) Q_DECL_OVERRIDE {Q_UNUSED(p); return false;}

	//void setAlignment(const Qt::Alignment &al) { alignmentFlags = al;}

	QString dump(int indent = 0) Q_DECL_OVERRIDE;
public:
	ReportItemMetaPaintText(ReportItemMetaPaint *parent, ReportItem *report_item)
	: ReportItemMetaPaint(parent, report_item) {}
	~ReportItemMetaPaintText() Q_DECL_OVERRIDE {}
};


class QFQMLWIDGETS_DECL_EXPORT ReportItemMetaPaintCheck : public ReportItemMetaPaintText
{
private:
	typedef ReportItemMetaPaintText Super;
public:
	virtual void paint(ReportPainter *painter, unsigned mode = PaintAll);
	virtual bool isExpandable() const {return false;}
public:
	ReportItemMetaPaintCheck(ReportItemMetaPaint *parent, ReportItem *report_item)
	: ReportItemMetaPaintText(parent, report_item) {}
};


class QFQMLWIDGETS_DECL_EXPORT ReportItemMetaPaintImage : public ReportItemMetaPaint
{
private:
	typedef ReportItemMetaPaintImage Super;
public:
	ReportItem::Image image;
	Qt::AspectRatioMode aspectRatioMode;
	//bool resize;
public:
	virtual void paint(ReportPainter *painter, unsigned mode = PaintAll);
	virtual bool isPointInside(const QPointF &p) {Q_UNUSED(p); return false;}

	virtual QString dump(int indent = 0);
public:
	ReportItemMetaPaintImage(ReportItemMetaPaint *parent, ReportItem *report_item)
	: ReportItemMetaPaint(parent, report_item), aspectRatioMode(Qt::IgnoreAspectRatio)/*, resize(true)*/ {}
};


class QFQMLWIDGETS_DECL_EXPORT ReportPainter : public QPainter
{
private:
	typedef QPainter Super;
public:
	ReportPainter(QPaintDevice *device);
public:
	ReportItemMetaPaint* selectedItem() const {return f_selectedItem;}
	void setSelectedItem(ReportItemMetaPaint *it) {f_selectedItem = it;}
	virtual void drawMetaPaint(ReportItemMetaPaint *item);
	//virtual void paintPage();
	bool isMarkEditableSqlText() const {return m_markEditableSqlText;}
	void setMarkEditableSqlText(bool b) {m_markEditableSqlText = b;}
public:
	/// field umoznujici zobrazit pocet stranek reportu, jinak to asi nejde, behem kompilace nevim, kolik jich nakonec bude.
	int pageCount;
protected:
	ReportItemMetaPaint *f_selectedItem;
	bool m_markEditableSqlText;
};

}}}

#endif // QF_QMLWIDGETS_REPORTS_REPORTPAINTER_H

