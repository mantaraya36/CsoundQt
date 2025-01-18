#include "midihandler.h"
#include "midilearndialog.h"

#ifdef CSQT_RTMIDI
#include "RtMidi.h"
#endif

#ifdef CSQT_RTMIDI
static void midiInMessageCallback(double deltatime,
								std::vector< unsigned char > *message,
								void *userData)
{
	Q_UNUSED(deltatime);
	MidiHandler *midiHandler = (MidiHandler *) userData;
	midiHandler->passMidiMessage(message);
	//  if (nBytes > 0) {
	//    qDebug() << "stamp = " << deltatime;
	//  }
}

#endif


MidiHandler::MidiHandler(int api, QObject *parent) :
    QObject(parent)
{
#ifdef CSQT_RTMIDI
	qDebug()<<"Using RtMidi API: " << api;
	m_midiin = new RtMidiIn((RtMidi::Api) api, "CsoundQt"); //api - see RtMidi.h for types
	m_midiout = new RtMidiOut((RtMidi::Api) api, "CsoundQt");
	m_midiin->setCallback(&midiInMessageCallback, this);

#endif
	m_midiLearnDialog = NULL;
}

void MidiHandler::addListener(DocumentPage *page)
{
	if(!m_listeners.contains(page)) {
		m_listeners.append(page);
	}
}

void MidiHandler::removeListener(DocumentPage *page)
{
	if(m_listeners.contains(page)) {
		m_listeners.remove(m_listeners.indexOf(page));
	}
}

void MidiHandler::setListener(DocumentPage *page)
{
	m_listeners.clear();
	m_listeners.append(page);
}

void MidiHandler::setMidiLearner(MidiLearnDialog *midiLearn)
{
	m_midiLearnDialog = midiLearn;
}

void MidiHandler::passMidiMessage(std::vector<unsigned char> *message)
{
	if (!message) {
		qDebug() << "MidiHandler::passMidiMessage Error: message is NULL";
		return;
	}
	foreach(DocumentPage *page, m_listeners) {		
		page->queueMidiIn(message);
	}
	if (m_midiLearnDialog) {
		if (message->size() > 2 && ((*message)[0] & 0x90)) {
			m_midiLearnDialog->setMidiController(((*message)[0] & 0x0F) + 1, (*message)[1] & 0x7F); // was & 8F, that exludes most controller numbers above 0xf
		}
	}
}

void MidiHandler::sendMidiOut(std::vector<unsigned char> *message)
{
#ifdef CSQT_RTMIDI
	m_midiout->sendMessage(message);
#else
    (void) message;
#endif
}


void MidiHandler::setMidiInterface(int number)
{
	if (number >= 0 && number < 9998) {
		openMidiInPort(number);
	}
	else {
		closeMidiInPort();
	}
}

int MidiHandler::findMidiInPortByName(QString name) {
	int port = 9999; // stands for None
#ifdef CSQT_RTMIDI
	// to check agains alsa names wihtout port
    static const QRegularExpression re("(\\s\\d+):(\\d+)$");
    int index = name.indexOf(re);
	if (index>0) {
		qDebug()<<name << " seems to be Alsa device. Excluding port part.";
		name=name.left(index); // remove also space
	}
	QString portName;
    for (unsigned int i=0; i<m_midiin->getPortCount(); i++) { // find port number according to the name
		portName = QString::fromStdString(m_midiin->getPortName(i));
		// for alsa portnames come in formant <portname> <dd:dd> - get rid of the last part to match the device if i

		if (portName.startsWith(name)) {
			qDebug()<<"Found port for " << name << ": " << i;
			port = i;
			break;
		}
	}
#endif
	return port;
}

int MidiHandler::findMidiOutPortByName(QString name) {
	int port = 9999; // stands for None
#ifdef CSQT_RTMIDI
    static const QRegularExpression re("(\\s\\d+):(\\d+)$");
    int index = name.indexOf(re);
	if (index>0) {
		qDebug()<<name << " seems to be Alsa device. Excluding port part.";
		name=name.left(index);
	}
	QString portName;
    for (unsigned int i=0; i<m_midiout->getPortCount(); i++) { // find port number according to the name
		portName = QString::fromStdString(m_midiout->getPortName(i));
		//qDebug()<<i<< " " << portName;
		if (portName.startsWith(name)) {
			qDebug()<<"Found port for " << name << ": " << i;
			port = i;
			break;
		}
	}
#endif
	return port;
}


void MidiHandler::openMidiInPort(int port)
{
    (void) port;
#ifdef CSQT_RTMIDI
	try {
		closeMidiInPort();
	}
#ifdef CSQT_OLD_RTMIDI
	catch (RtError &error) {
#else
	catch (RtMidiError &error) {
#endif
		error.printMessage();
	}

	try {
		m_midiin->openPort(port, "MIDI in");
	}
#ifdef CSQT_OLD_RTMIDI
	catch (RtError &error) {
#else
	catch (RtMidiError &error) {
#endif
		qDebug() << "Error opening MIDI port " << port;
		error.printMessage();
		return;
	}
	qDebug() << "CsoundQt::openMidiPort opened port " << port;
#endif
	//  m_midiin->ignoreTypes(false, false, false);
}

void MidiHandler::setMidiOutInterface(int number)
{
	if (number >= 0 && number < 9998) {
		openMidiOutPort(number);
	}
	else {
		closeMidiOutPort();
	}
}

void MidiHandler::openMidiOutPort(int port)
{
    (void) port;
#ifdef CSQT_RTMIDI
	try {
		closeMidiOutPort();
	}
#ifdef CSQT_OLD_RTMIDI
	catch (RtError &error) {
#else
	catch (RtMidiError &error) {
#endif
		error.printMessage();
	}

	try {
		m_midiout->openPort(port, "MIDI out");
	}
#ifdef CSQT_OLD_RTMIDI
	catch (RtError &error) {
#else
	catch (RtMidiError &error) {
#endif
		qDebug() << "Error opening MIDI out port " << port;
		error.printMessage();
		return;
	}
	  qDebug() << "CsoundQt::openMidiOutPort opened port " << port;
#endif
	//  m_midiin->ignoreTypes(false, false, false);

}

void MidiHandler::closeMidiInPort()
{
#ifdef CSQT_RTMIDI
	m_midiin->closePort();
#endif
}

void MidiHandler::closeMidiOutPort()
{
#ifdef CSQT_RTMIDI
	m_midiout->closePort();
#endif
}
