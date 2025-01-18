#ifndef HTML5GUIDISPLAY_H
#define HTML5GUIDISPLAY_H

#if defined(CSQT_QTHTML)

#include <atomic>
#include <QDebug>
#include <QDockWidget>
#include "csoundhtmlwrapper.h"
#include "CsoundHtmlOnlyWrapper.h"
#include <QtWebEngineWidgets>
#include <QtWebChannel/QtWebChannel>
#include <QWebEngineView>
#include <QTemporaryFile>


namespace Ui {
class Html5GuiDisplay;
}

class DocumentPage;

class CsoundHtmlView : public QDockWidget
{
	Q_OBJECT
public:
	explicit CsoundHtmlView(QWidget *parent = 0);
	~CsoundHtmlView();
    void loadFromUrl(const QUrl &url);
    void load(DocumentPage *documentPage);
    void stop();
    void setCsoundEngine(CsoundEngine *csEngine);
	void clear();
    void setOptions(CsoundOptions * options);
	QWebChannel channel ;            // Channel for C++ to Javascript communications
    QWebEngineView *webView;	


public slots:
	void showDebugWindow();
    void removeTemporaryHtmlFile(bool ok);



private:
	Ui::Html5GuiDisplay *ui;
    std::atomic<DocumentPage *> documentPage;
    // For performing CSD files with embedded <html> element.
    CsoundHtmlWrapper csoundHtmlWrapper;
    // For performing HTML files (HTML-only performance).
    CsoundHtmlOnlyWrapper csoundHtmlOnlyWrapper;
    CsoundEngine *m_csoundEngine;
    //QTemporaryFile tempHtml;
    QFile htmlFile;
    CsoundOptions * m_options;
	QString m_debugPort;

};

#endif
#endif // HTML5GUIDISPLAY_H
