#ifndef MIDIHANDLER_H
#define MIDIHANDLER_H

#include <QObject>

#include "documentpage.h"

class RtMidiIn;
class RtMidiOut;

class MidiHandler : public QObject
{
    Q_OBJECT
public:
	explicit MidiHandler(int api=0, QObject *parent = 0);
	void setMidiInterface(int number);
	void setMidiInterface(QString name);
	int findMidiInPortByName(QString name);
	int findMidiOutPortByName(QString name);
	void openMidiInPort(int port);
    void setMidiOutInterface(int number);
    void openMidiOutPort(int port);
    void closeMidiInPort();
    void closeMidiOutPort();

    void addListener(DocumentPage *page);
    void removeListener(DocumentPage *page);
    void setListener(DocumentPage *page); // Unique listener

    void setMidiLearner(MidiLearnDialog *midiLearn);

    void passMidiMessage(std::vector< unsigned char > *message);
    void sendMidiOut(std::vector< unsigned char > *message);


signals:

public slots:
private:
	QVector<DocumentPage *> m_listeners;
	MidiLearnDialog *m_midiLearnDialog;

#ifdef CSQT_RTMIDI
	RtMidiIn *m_midiin;
	RtMidiOut *m_midiout;
#endif

};

#endif // MIDIHANDLER_H
