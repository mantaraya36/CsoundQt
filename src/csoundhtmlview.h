#ifndef HTML5GUIDISPLAY_H
#define HTML5GUIDISPLAY_H

#if defined(CSQT_QTHTML)

#include <atomic>
#include <QDebug>
#include <QDockWidget>
#include "csoundhtmlwrapper.h"
#include "CsoundHtmlOnlyWrapper.h"

#ifdef USE_WEBKIT
	#include <QtWebKit>
	#include <QWebView>
	#include <QWebFrame>
	#include <QWebInspector>
#endif
#ifdef USE_WEBENGINE
	#include <QtWebEngineWidgets>
	#include <QtWebChannel/QtWebChannel>
	#include <QWebEngineView>
#endif


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
#ifdef USE_WEBKIT
	QWebView *webView;
#endif
#ifdef USE_WEBENGINE
	QWebChannel channel ;            // Channel for C++ to Javascript communications
    QWebEngineView *webView;	
#endif

public slots:
#ifdef USE_WEBKIT
	void addJSObject();
#endif
#ifdef USE_WEBENGINE
	void showDebugWindow();
#endif
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
#ifdef USE_WEBENGINE
	QString m_debugPort;
#endif
};

#endif
#endif // HTML5GUIDISPLAY_H
