#include "midilearndialog.h"
#include "ui_midilearndialog.h"

#include <QDebug>

MidiLearnDialog::MidiLearnDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MidiLearnDialog)
{
    ui->setupUi(this);
    m_widget = 0;
	m_cc = -1; m_channel = -1;
}

MidiLearnDialog::~MidiLearnDialog()
{
    delete ui;
}

void MidiLearnDialog::setCurrentWidget(QuteWidget *widget)
{
    m_widget = widget;
    if (widget) {
        if (widget->acceptsMidi()) {
            int cc = m_widget->property("CSQT_midicc").toInt();
            int channel = m_widget->property("CSQT_midichan").toInt();
            if (channel <= 0) {
                ui->channelLabel->setText(QString::number(channel));
                ui->ccLabel->setText(QString::number(cc));
            } else {
                ui->channelLabel->setText(tr("(None)"));
                ui->ccLabel->setText(tr("(None)"));
            }
        } else {
            ui->channelLabel->setText(tr("(No MIDI)"));
            ui->ccLabel->setText(tr("(No MIDI)"));
        }
		//qDebug() << "MidiLearnDialog::setCurrentWidget";
    } else {
            ui->channelLabel->setText(tr("(None)"));
            ui->ccLabel->setText(tr("(None)"));
    }
}

//void MidiLearnDialog::closeEvent(QCloseEvent *event)
//{
//    qDebug() << "MIDI learn closed";

//}

void MidiLearnDialog::setMidiController(int channel, int cc)
{
	m_channel = channel; m_cc = cc;
	ui->channelLabel->setText(QString::number(channel));
	ui->ccLabel->setText(QString::number(cc));
}

void MidiLearnDialog::on_setButton_clicked()
{
	if (m_cc<0 || m_channel<0 ) {
        QMessageBox::warning(this, tr("Controller not set"), tr("MIDI controller is not selected!"));
		return;
	}
	if (m_widget && m_widget->acceptsMidi()) {
		m_widget->setProperty("CSQT_midicc", m_cc);
		m_widget->setProperty("CSQT_midichan", m_channel);
		m_widget->applyInternalProperties();
		m_widget->markChanged();

		m_widget->updateDialogWindow(m_cc,m_channel);
	}
}

void MidiLearnDialog::on_cancelButton_clicked()
{
	this->close();
}

void MidiLearnDialog::on_closeButton_clicked()
{
	on_setButton_clicked();
	this->close();
}
