#if defined(CSQT_QTHTML)
#include "basedocument.h"
#include "documentpage.h"
#include "csoundhtmlview.h"
#include "ui_html5guidisplay.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWaitCondition>
#include <QFile>
#include <QObject>

CsoundHtmlView::CsoundHtmlView(QWidget *parent) :
    QDockWidget(parent),
    webView(nullptr),
    ui(new Ui::Html5GuiDisplay),
    documentPage(0),
    m_csoundEngine(nullptr),
	m_options(nullptr)
{
    ui->setupUi(this);

    webView = new QWebEngineView(this);
    //webView->page()->profile()->clearHttpCache();
    QObject::connect(webView, &QWebEngineView::loadFinished, this, &CsoundHtmlView::removeTemporaryHtmlFile );
    csoundHtmlWrapper.setCsoundHtmlView(this);
    csoundHtmlOnlyWrapper.setCsoundHtmlView(this);
	ui->mainLayout->addWidget(webView); // mainLayout is vertical layout box
    webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Enable dev tools by default for the test browser
	connect(ui->inspectButton, SIGNAL(clicked()),this, SLOT(showDebugWindow()));
    webView->page()->setWebChannel(&channel);
    //qDebug() << "Setting JavaScript object on init.";
    channel.registerObject("csound", &csoundHtmlWrapper);
}

CsoundHtmlView::~CsoundHtmlView()
{
    delete ui;
}

QString getElement(const QString &text, const QString &tag)
{
    QString::SectionFlags sectionflags = QString::SectionIncludeLeadingSep | QString::SectionIncludeTrailingSep | QString::SectionCaseInsensitiveSeps;
    QString element = text.section("<" + tag, 1, 1, sectionflags);
    element = element.section("</" + tag + ">", 0, 0, sectionflags);
    return element;
}

void CsoundHtmlView::load(DocumentPage *documentPage_)
{
    qDebug();
    documentPage = documentPage_;
    auto text = documentPage.load()->getFullText();
    auto filename = documentPage.load()->getFileName();
    QFile csdfile(filename);
    csdfile.open(QIODevice::WriteOnly);
    QTextStream out(&csdfile);
    out << text;
    csdfile.close();
    auto html = getElement(text, "html");
    if (html.size() > 0) {
        // Inject necessary code to load qtwebchannel/qwebchannel.js.
        QString injection = R"(
<script type="text/javascript" src="qrc:///qtwebchannel/qwebchannel.js"></script>
<script type="text/javascript">
"use strict";
document.addEventListener("DOMContentLoaded", function () {
    try {
        console.log("Initializing window.csound...");
        window.channel = new QWebChannel(qt.webChannelTransport, function(channel) {
        window.csound = channel.objects.csound;
        if (typeof window.csound === 'undefined') {
            alert('window.csound is undefined.');
            return;
        }
        if (window.csound === null) {
            alert('window.csound is null.');
            return;
        }
        csound.message("Initialized csound.\n");
        });
    } catch (e) {
        alert("initialize_csound error: " + e.message);
        console.log(e.message);
    }
});
</script>
)";
        // Tricky because now HTML doesn't have to have a <head> element,
        // and both <html> and <head> can have attributes. So we need to find an
        // injection point that is the very first place allowed to put a <script>
        // element.
        int injection_index = html.indexOf("<head", 0, Qt::CaseInsensitive);
        if (injection_index != -1) {
            injection_index = html.indexOf(">", injection_index) + 1;
        } else {
            injection_index = html.indexOf("<html", 0, Qt::CaseInsensitive);
            injection_index = html.indexOf(">", injection_index) + 1;
        }
        html = html.insert(injection_index, injection);

        QString htmlFilename; // maybe use also tempHtml herer

        QFileInfo fileInfo(QFileInfo(filename).path());


        if (filename.startsWith(":/") || !fileInfo.isWritable()) { // if an example file or the folder is not writable
            htmlFilename = QDir::tempPath()+"/html-example.html"; // create it in a temporary location
        } else {
            htmlFilename = filename + ".html";
        }
        htmlFile.setFileName(htmlFilename);
        htmlFile.open(QIODevice::WriteOnly);
        QTextStream out(&htmlFile);
        out << html;
        htmlFile.close();
        loadFromUrl(QUrl::fromLocalFile(htmlFilename));

        webView->page()->setWebChannel(&channel);
        if (filename.endsWith(".html", Qt::CaseInsensitive)) {
            // Register CsoundHtmlOnlyWrapper when performing HTML files.
            qDebug()  << "Setting CsoundHtmlOnlyWrapper JavaScript object on load.";
            channel.registerObject("csound", &csoundHtmlOnlyWrapper);
            csoundHtmlOnlyWrapper.registerConsole(documentPage_->getConsole());
            csoundHtmlOnlyWrapper.setOptions(m_options);
        } else {
            // Register CsoundHtmlWrapper when performing CSD files with embedded <html> element.
            qDebug()  << "Setting CsoundWrapper JavaScript object on load.";
            channel.registerObject("csound", &csoundHtmlWrapper);
        }
    }
    repaint();
}

void CsoundHtmlView::setCsoundEngine(CsoundEngine *csEngine)
{
    csoundHtmlWrapper.setCsoundEngine(csEngine);
}


void CsoundHtmlView::stop()
{
    qDebug() ;
    documentPage = 0;
    csoundHtmlOnlyWrapper.stop();
}


#ifdef USE_WEBKIT
void CsoundHtmlView::addJSObject()
{
	if (webView) {
        QString filename = documentPage.load()->getFileName();
        qDebug()<<"Adding Csound as JavaScript object";
        if (filename.endsWith(".html", Qt::CaseInsensitive)) {
            // Register CsoundHtmlOnlyWrapper when performing HTML files.
            webView->page()->mainFrame()->addToJavaScriptWindowObject("csound", &csoundHtmlOnlyWrapper);
            csoundHtmlOnlyWrapper.registerConsole(documentPage.load()->getConsole());
            csoundHtmlOnlyWrapper.setOptions(m_options);
        } else {
            // Register CsoundHtmlWrapper when performing CSD files with embedded <html> element.
			webView->page()->mainFrame()->addToJavaScriptWindowObject("csound", &csoundHtmlWrapper);
        }


	}



}
#endif

void CsoundHtmlView::loadFromUrl(const QUrl &url)
{
    qDebug();

    if(webView != 0) {
        webView->setUrl(url);
    }
    if (!this->isVisible()) {
        this->show();
        this->raise();
    }
}

void CsoundHtmlView::clear()
{
    this->hide(); // just hide the panel, keep qwebchannel and other connections
    //loadFromUrl(QUrl()); // empty URL to clear -  this causes "qt is not defined" error
}

void CsoundHtmlView::setOptions(CsoundOptions *options)
{
	m_options = options;
}

void CsoundHtmlView::showDebugWindow()
{
	qDebug();
	QByteArray debugPort = qgetenv("QTWEBENGINE_REMOTE_DEBUGGING");
	if (!debugPort.isNull()) {
		QWidget * debugger = new QWidget();
		debugger->resize(600,400);
		QWebEngineView * debuggerView= new QWebEngineView(debugger);
        QVBoxLayout *layout = new QVBoxLayout(debugger);
        layout->addWidget(debuggerView);
        debugger->setAttribute(Qt::WA_DeleteOnClose);
        debugger->setLayout(layout);
		qDebug()<<"Opening window for localhost:"<<debugPort;
        debuggerView->setUrl(QUrl("http://localhost:"+debugPort));
		debugger->show();
	} else {
        qDebug()<<"Debugging port not set or reading failed";
    }
}

void CsoundHtmlView::removeTemporaryHtmlFile(bool ok)
{
    if (ok) {
        qDebug() << "Removing temporary html: " << htmlFile.fileName();
        htmlFile.remove();

    }
}

#endif
