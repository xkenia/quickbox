#include "receiptsprinter.h"
#include "Receipts/receiptsplugin.h"

#include <qf/core/collator.h>

#include <qf/qmlwidgets/framework/mainwindow.h>
#include <qf/qmlwidgets/reports/processor/reportpainter.h>
#include <qf/qmlwidgets/reports/processor/reportprocessor.h>

#include <QCryptographicHash>
#include <QDomDocument>
#include <QPrinter>
#include <QPrinterInfo>
#include <QTcpSocket>
#include <QTextCodec>
#include <QUdpSocket>

//#define QF_TIMESCOPE_ENABLED
#include <qf/core/utils/fileutils.h>
#include <qf/core/utils/timescope.h>

namespace qfu = qf::core::utils;
namespace qff = qf::qmlwidgets::framework;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
static const auto SkipEmptyParts = QString::SkipEmptyParts;
#else
static const auto SkipEmptyParts = Qt::SkipEmptyParts;
#endif

ReceiptsPrinter::ReceiptsPrinter(const ReceiptsPrinterOptions &opts, QObject *parent)
	: QObject(parent)
	, m_printerOptions(opts)
{
}
/*
static Receipts::ReceiptsPlugin *receiptsPlugin()
{
	qff::MainWindow *fwk = qff::MainWindow::frameWork();
	auto *ret = qobject_cast<Receipts::ReceiptsPlugin *>(fwk->plugin("Receipts"));
	QF_ASSERT(ret != nullptr, "Bad plugin", return 0);
	return ret;
}
*/
void ReceiptsPrinter::printReceipt(const QString &report_file_name, const QVariantMap &report_data)
{
	qfLogFuncFrame();
	if(report_file_name.isEmpty()) {
		qfError() << "Empty receipt path.";
		return;
	}
	QF_TIME_SCOPE("ReceiptsPrinter::printReceipt()");
	const ReceiptsPrinterOptions &printer_opts = m_printerOptions;
	QPrinter *printer = nullptr;
	QPaintDevice *paint_device = nullptr;
	if(printer_opts.printerType() == (int)ReceiptsPrinterOptions::PrinterType::GraphicPrinter) {
		QF_TIME_SCOPE("init graphics printer");
		QPrinterInfo pi = QPrinterInfo::printerInfo(printer_opts.graphicsPrinterName());
		if(pi.isNull()) {
			for(auto s : QPrinterInfo::availablePrinterNames()) {
				qfInfo() << "available printer:" << s;
			}
			pi = QPrinterInfo::defaultPrinter();
		}
		if(pi.isNull()) {
			qfWarning() << "Default printer not set";
			return;
		}
		qfInfo() << "printing on:" << pi.printerName();
		printer = new QPrinter(pi);
		paint_device = printer;
	}
	else {
		qfInfo() << "printing on:" << printer_opts.characterPrinterModel() << "at:"
				 << ((printer_opts.characterPrinterType() == ReceiptsPrinterOptions::CharacterPrinteType::Directory)?
						 printer_opts.characterPrinterDirectory() :
						 printer_opts.characterPrinterDevice());
		qff::MainWindow *fwk = qff::MainWindow::frameWork();
		paint_device = fwk;
	}
	qf::qmlwidgets::reports::ReportProcessor rp(paint_device);
	{
		QF_TIME_SCOPE("setting report and data");
		if(!rp.setReport(report_file_name))
			return;
		for(auto key : report_data.keys()) {
			rp.setTableData(key, report_data.value(key));
		}
	}
	if(printer_opts.printerType() == ReceiptsPrinterOptions::PrinterType::GraphicPrinter) {
		QF_TIME_SCOPE("process graphics");
		{
			QF_TIME_SCOPE("process report");
			rp.process();
		}
		qf::qmlwidgets::reports::ReportItemMetaPaintReport *doc;
		{
			QF_TIME_SCOPE("getting processor output");
			doc = rp.processorOutput();
		}
		qf::qmlwidgets::reports::ReportItemMetaPaint *it = doc->child(0);
		if(it) {
			QF_TIME_SCOPE("draw meta-paint");
			qf::qmlwidgets::reports::ReportPainter painter(paint_device);
			painter.drawMetaPaint(it);
		}
		QF_SAFE_DELETE(printer);
	}
	else if(printer_opts.printerType() == ReceiptsPrinterOptions::PrinterType::CharacterPrinter) {
		QDomDocument doc;
		doc.setContent(QLatin1String("<?xml version=\"1.0\"?><report><body/></report>"));
		QDomElement el_body = doc.documentElement().firstChildElement("body");
		qf::qmlwidgets::reports::ReportProcessor::HtmlExportOptions opts;
		opts.setConvertBandsToTables(false);
		rp.processHtml(el_body, opts);
		//qfInfo() << doc.toString();
		QList<QByteArray> data_lines = createPrinterData(el_body, printer_opts);
		auto save_file = [data_lines](const QString &fn) {
			QFile f(fn);
			if(f.open(QFile::WriteOnly)) {
				//qfInfo() << "writing receipt data to:" << fn;
				for(QByteArray ba : data_lines) {
					f.write(ba);
					f.write("\n");
				}
			}
			else {
				qfError() << "Cannot open file" << f.fileName() << "for writing!";
			}
		};
		switch(printer_opts.characterPrinterType()) {
			case ReceiptsPrinterOptions::CharacterPrinteType::Directory: {
				if(!printer_opts.characterPrinterDirectory().isEmpty()) {
					QString fn = printer_opts.characterPrinterDirectory();
					qf::core::utils::FileUtils::ensurePath(fn);
					QCryptographicHash ch(QCryptographicHash::Sha1);
					for(QByteArray ba : data_lines)
						ch.addData(ba);
					fn += '/' + QString::fromLatin1(ch.result().toHex().mid(0, 8)) + ".txt";
					save_file(fn);
				}
				break;
			}
			case ReceiptsPrinterOptions::CharacterPrinteType::LPT: {
				if (!printer_opts.characterPrinterDevice().isEmpty()) {
					save_file(printer_opts.characterPrinterDevice());
				}
				break;
			}
			case ReceiptsPrinterOptions::CharacterPrinteType::Network: {
				if (!printer_opts.characterPrinterUrl().isEmpty()) {
					QString host = printer_opts.characterPrinterUrl().section(':', 0, 0);
					QHostAddress host_addr(host);
					int port = printer_opts.characterPrinterUrl().section(':', 1, 1).toInt();
					if(printer_opts.isCharacterPrinterUdpProtocol()) {
						QByteArray dgram;
						for(const QByteArray& line : data_lines) {
							dgram += line;
							dgram += '\n';
						}
						// v Racomu chteli pridat 2 radky, aby jim to vyjelo vic,
						// spravne by to asi resil custom footer u vycetky
						dgram += "\n\n\n";
						QUdpSocket socket;
						socket.writeDatagram(dgram, host_addr, port);
					}
					else {
						QTcpSocket socket;
						socket.connectToHost(host_addr, port, QIODevice::WriteOnly);
						if (socket.waitForConnected(1000)) {
							for(const QByteArray& line : data_lines) {
								socket.write(line);
								socket.write("\n");
							}
							socket.disconnectFromHost();
							if (!socket.waitForDisconnected(1000)) { // waiting till all data are sent
								qfError() << "Error while closing the connection to printer: "
										<< socket.error();
							}
						}
						else {
							qfError() << "Cannot open tcp connection to address "
									<< host << " on port " << port
									<< " reason: " << socket.error();
						}
					}
				}
				break;
			}
		}
	}
}

static const QByteArray epson_commands[] =
{
	QByteArray("\033E\x01"),		//    BoldOn
	QByteArray("\033E\x00", 3),		//    BoldOff
	QByteArray("\033-\x01"),		//    UnderlineOn
	QByteArray("\033-\x00", 3),		//    UnderlineOff
	QByteArray("\033-\x02"),		//    Underline2On
	QByteArray("\033-\x00", 3),		//    Underline2Off
	QByteArray("\033a\x00", 3),		//    AlignLeft
	QByteArray("\033a\x01", 3),		//    AlignCenter
	QByteArray("\033a\x02", 3),		//    AlignRight
	QByteArray("\033@"),			//    Init
	QByteArray("\035V\102\x00", 4),	//    Cut
	QByteArray("\n"),				//    Eoln
};

struct PrintData
{
	enum class Command : int {
		BoldOn = 0,
		BoldOff,
		UnderlineOn,
		UnderlineOff,
		Underline2On,
		Underline2Off,
		AlignLeft,
		AlignCenter,
		AlignRight,
		Init,
		Cut,
		Eoln,
		HorizontalLine,
		Text,
	};
	Command command;
	QByteArray data;
	int width = 0;
	Qt::Alignment alignment = Qt::AlignLeft;

	PrintData(Command cmd, const QByteArray &data = QByteArray(), int width = 0, Qt::Alignment alignment = Qt::AlignLeft)
		: command(cmd), data(data), width(width), alignment(alignment)
	{}
	bool isCommand() const {return command != Command::Text;}
	int textLength() const {return isCommand()? 0: data.length();}
	const QByteArray toByteArray() const
	{
		QByteArray ret;
		switch(command) {
		case Command::BoldOn: ret += "<B>"; break;
		case Command::BoldOff: ret += "</B>"; break;
		case Command::UnderlineOn: ret += "<U>"; break;
		case Command::UnderlineOff: ret += "</U>:"; break;
		case Command::Underline2On: ret += "<u>:"; break;
		case Command::Underline2Off: ret += "</u>:"; break;
		case Command::AlignLeft: ret += "<|-->"; break;
		case Command::AlignCenter: ret += "<-|->"; break;
		case Command::AlignRight: ret += "<--|>"; break;
		case Command::Init: ret += "<INIT>"; break;
		case Command::Cut: ret += "<CUT>"; break;
		case Command::Eoln: ret += "<EOL>"; break;
		case Command::HorizontalLine: ret += "<HR>"; break;
		case Command::Text:
			if(width == 0)
				ret += data;
			else
				ret += "<" + (width < 0? "%:": QByteArray::number(width) + ":") + data + ">";
			break;
		}
		return ret;
	}
};
typedef QList<PrintData> PrintLine;

class DirectPrintContext
{
public:
	PrintLine line;
	int horizontalLayoutNestCount = 0;
	//int printerLineWidth = 42;
};

void ReceiptsPrinter::createPrinterData_helper(const QDomElement &el, DirectPrintContext *print_context, const QString &text_encoding)
{
	//QByteArray text;
	PrintLine pre_commands;
	PrintLine post_commands;
	int text_width = 0;
	bool is_halign = el.tagName() == QLatin1String("div")
			&& el.attribute(qf::qmlwidgets::reports::ReportProcessor::HTML_ATTRIBUTE_LAYOUT) == QLatin1String("horizontal");
	if(is_halign)
		print_context->horizontalLayoutNestCount++;
	QDomNamedNodeMap attrs = el.attributes();
	for (int i = 0; i < attrs.count(); ++i) {
		QDomAttr attr = attrs.item(i).toAttr();
		QString key = attr.name();
		if(key == QLatin1String("lpt_textWidth")) {
			QString val = attr.value();
			if(val == "%")
				text_width = -1;
			else
				text_width = val.toInt();
		}
		else if(key == QLatin1String("lpt_textStyle")) {
			QStringList styles = attr.value().split(',', SkipEmptyParts);
			for(QString s : styles) {
				s = s.trimmed();
				if(s == QLatin1String("bold")) {
					pre_commands << PrintData(PrintData::Command::BoldOn);
					post_commands.insert(0, PrintData(PrintData::Command::BoldOff));
				}
				else if(s == QLatin1String("underline")) {
					pre_commands << PrintData(PrintData::Command::UnderlineOn);
					post_commands.insert(0, PrintData(PrintData::Command::UnderlineOff));
				}
				else if(s == QLatin1String("underline2")) {
					pre_commands << PrintData(PrintData::Command::Underline2On);
					post_commands.insert(0, PrintData(PrintData::Command::Underline2Off));
				}
			}
		}
		else if(key == QLatin1String("lpt_textAlign") && el.tagName() != QLatin1String("p")) {
			QString val = attr.value();
			if(val == QLatin1String("right")) {
				pre_commands << PrintData(PrintData::Command::AlignRight);
				post_commands.insert(0, PrintData(PrintData::Command::AlignLeft));
			}
			else if(val == QLatin1String("center")) {
				pre_commands << PrintData(PrintData::Command::AlignCenter);
				post_commands.insert(0, PrintData(PrintData::Command::AlignLeft));
			}
			else if(val == QLatin1String("left")) {
				pre_commands << PrintData(PrintData::Command::AlignLeft);
				post_commands.insert(0, PrintData(PrintData::Command::AlignLeft));
			}
		}
		else if(key == QLatin1String("lpt_borderTop")) {
			QString val = attr.value();
			pre_commands << PrintData(PrintData::Command::HorizontalLine, val.toUtf8());
		}
		else if(key == QLatin1String("lpt_borderBottom")) {
			QString val = attr.value();
			post_commands << PrintData(PrintData::Command::HorizontalLine, val.toUtf8());
		}
	}
	print_context->line << pre_commands;
	if(el.tagName() == QLatin1String("p")) {
		QString ta = el.attribute(QLatin1String("lpt_textAlign"));
		Qt::Alignment text_align = Qt::AlignLeft;
		if(ta == QLatin1String("right"))
			text_align = Qt::AlignRight;
		else if(ta == QLatin1String("center"))
			text_align = Qt::AlignHCenter;
		QByteArray text;
		QTextCodec *tc = nullptr;
		if(text_encoding != QLatin1String("ASCII7")) {
			QByteArray ba = text_encoding.toUtf8();
			tc = QTextCodec::codecForName(ba);
		}
		if(tc)
			text = tc->fromUnicode(el.text());
		else
			text = qf::core::Collator::toAscii7(QLocale::Czech, el.text(), false);
		print_context->line << PrintData(PrintData::Command::Text, text, text_width, text_align);
	}
	{
		for(QDomElement el1 = el.firstChildElement(); !el1.isNull(); ) {
			createPrinterData_helper(el1, print_context, text_encoding);
			el1 = el1.nextSiblingElement();
			if(!el1.isNull()) {
				//if(!is_halign || (is_halign && print_context->horizontalLayoutNestCount == 1)) {
				if(!is_halign) {
					print_context->line << PrintData(PrintData::Command::Eoln);
				}
			}
		}
	}
	print_context->line << post_commands;
	if(is_halign)
		print_context->horizontalLayoutNestCount--;
}

static QList<PrintLine> alignPrinterData(DirectPrintContext *print_context, const ReceiptsPrinterOptions &printer_options)
{
	QList<PrintLine> ret;
	int line_length = printer_options.characterPrinterLineLength();
	PrintLine line;
	{
		line << PrintData(PrintData::Command::Init);
		ret.insert(ret.length(), line);
	}
	line.clear();
	for (int i = 0; i < print_context->line.count(); ++i) {
		const PrintData &pd = print_context->line[i];
		bool is_eol = (pd.command == PrintData::Command::Eoln) || (i == (print_context->line.count() - 1));
		if(pd.command != PrintData::Command::Eoln) {
			line << pd;
		}
		if(is_eol) {
			int fixed_text_len = 0;
			int spring_cnt = 0;
			for (int j = 0; j < line.length(); ++j) {
				const PrintData &pd2 = line[j];
				if(pd2.width < 0)
					spring_cnt++;
				else if(pd2.width > 0)
					fixed_text_len += pd2.width;
				else
					fixed_text_len += pd2.textLength();
			}
			for (int j = 0; j < line.length(); ++j) {
				PrintData &pd2 = line[j];
				if(pd2.isCommand())
					continue;
				int w = pd2.width;
				if(w < 0)
					w = (line_length - fixed_text_len) / spring_cnt;
				if(w > 0) {
					int w_rest = w - pd2.textLength();
					if(w_rest > 0) {
						if(pd2.alignment == Qt::AlignLeft)
							pd2.data = pd2.data + QByteArray(w_rest, ' ');
						else if(pd2.alignment == Qt::AlignRight)
							pd2.data = QByteArray(w_rest, ' ') + pd2.data;
						else if(pd2.alignment == Qt::AlignHCenter)
							pd2.data = QByteArray(w_rest/2+1, ' ') + pd2.data + QByteArray(w_rest/2+1, ' ');
					}
					pd2.data = pd2.data.mid(0, w);
					pd2.width = 0;
				}
			}
			ret.insert(ret.length(), line);
			line.clear();
		}
	}
	{
		line.clear();
		line << PrintData(PrintData::Command::Cut);
		ret.insert(ret.length(), line);
	}
	return ret;
}

static QList<QByteArray> interpretControlCodes(const QList<PrintLine> &lines, const ReceiptsPrinterOptions &printer_options)
{
	QList<QByteArray> ret;
	int line_length = printer_options.characterPrinterLineLength();
	bool include_escapes = printer_options.isCharacterPrinterGenerateControlCodes();
	const int cmd_length = sizeof(epson_commands) / sizeof(char*);
	for(const PrintLine line : lines) {
		QByteArray ba;
		int line_text_len = 0;
		for(const PrintData &pd : line) {
			if(pd.command == PrintData::Command::Text) {
				ba += pd.data;
				line_text_len += pd.data.length();
			}
			else if(pd.command == PrintData::Command::HorizontalLine) {
				QByteArray hr_data(line_length, pd.data[0]);
				if(line_text_len == 0) {
					ret.insert(ret.length(), ba + hr_data);
					ba.clear();
				}
				else {
					ret.insert(ret.length(), ba);
					ba = hr_data;
					line_text_len = hr_data.length();
				}
			}
			else if(include_escapes) {
				int ix = (int)pd.command;
				QF_ASSERT(ix < cmd_length, QString("%1 - Bad command index!").arg(ix), continue);
				ba += epson_commands[ix];
			}
		}
		ret.insert(ret.length(), ba);
	}
	return ret;
}

QList<QByteArray> ReceiptsPrinter::createPrinterData(const QDomElement &body, const ReceiptsPrinterOptions &printer_options)
{
	DirectPrintContext dpc;
	//dpc.printerLineWidth = printer_options.characterPrinterLineLength();
	createPrinterData_helper(body, &dpc, printer_options.characterPrinterCodec());
	/*
	{
		QByteArray ba;
		for(auto d : dpc.line)
			ba += d.toByteArray();
		qfInfo() << ba;
	}
	*/
	QList<PrintLine> lines = alignPrinterData(&dpc, printer_options);
#if 0
	for(auto l : lines) {
		QByteArray ba;
		for(auto d : l)
			ba += d.toByteArray();
		qfDebug() << ba;
	}
#endif
	QList<QByteArray> ret = interpretControlCodes(lines, printer_options);
	//for(auto ba : ret) {
	//	qDebug() << ba;
	//}
	return ret;
}

