
#include "selectprobedialog.h"

using namespace probelib;

SelectProbeDialog::SelectProbeDialog(QWidget *parent) : QDialog(parent) {
    ui = new Ui::SelectProbeDialog();
    ui->setupUi(this);
}

SelectProbeDialog::~SelectProbeDialog() {
    delete ui;
}

bool SelectProbeDialog::execWithState(QVector<probelib::IProbeLib *> availableProbeLibs, IProbeLib *currentProbeLib,
                                      IAvailableProbe::p currentProbe) {
    // Save initial state
    m_availableProbeLibs = availableProbeLibs;
    m_currentProbeLib = currentProbeLib;
    m_currentProbe = currentProbe;

    // Populate UI
    {
        // Prevent combobox from firing signal for now
        QSignalBlocker blocker(ui->cmbProbeLib);
        foreach (auto probeLib, availableProbeLibs) {
            ui->cmbProbeLib->addItem(probeLib->name());
            if (probeLib == currentProbeLib) {
                ui->cmbProbeLib->setCurrentIndex(ui->cmbProbeLib->count() - 1);
            }
        }
    }

    // Let user see the dialog
    show();
    QApplication::processEvents();

    // This will take some time
    auto selectedProbeLib = currentProbeLib ? currentProbeLib : availableProbeLibs.first();
    selectProbeLib(selectedProbeLib);

    auto result = exec();

    if (!result)
        return false;

    return true;
}

void SelectProbeDialog::selectProbeLib(probelib::IProbeLib *selectedProbeLib) {
    QSignalBlocker blocker(ui->cmbProbeLib);

    m_selectedProbeLib = selectedProbeLib;
    ui->lblDesc->setText(selectedProbeLib->description());
    ui->lblVersion->setText(selectedProbeLib->version());
    ui->cmbProbeLib->setCurrentText(selectedProbeLib->name());

    // List probes from this probelib
    scanProbes(selectedProbeLib, m_currentProbe);
}

void SelectProbeDialog::scanProbes(IProbeLib *probeLib, IAvailableProbe::p currentProbe) {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_availableProbes = probeLib->availableProbes();
    ui->listProbes->clear();
    foreach (auto probe, m_availableProbes) {
        auto name = probe->name();
        auto serial = probe->serialNumber();
        // Mark a probe "selected" when name and serial number both matched
        if (currentProbe && name == currentProbe->name() && serial == currentProbe->serialNumber()) {
            name = tr("%1 (Selected)").arg(name);
            ui->listProbes->addItem(name);
            ui->listProbes->setCurrentRow(ui->listProbes->count() - 1);
        } else {
            ui->listProbes->addItem(name);
        }
    }

    QApplication::restoreOverrideCursor();
}

void SelectProbeDialog::on_cmbProbeLib_currentIndexChanged(int index) {
    selectProbeLib(m_availableProbeLibs[index]);
}

void SelectProbeDialog::on_listProbes_doubleClicked(QModelIndex index) {
    on_btnSelect_clicked();
}

void SelectProbeDialog::on_btnRescan_clicked() {
    ui->listProbes->clear();
    QApplication::processEvents(); // Let user see we deleted all items
    scanProbes(m_selectedProbeLib, m_currentProbe);
}

void SelectProbeDialog::on_btnSelect_clicked() {
    if (ui->listProbes->currentRow() < 0) {
        return;
    }

    m_selectedProbe = m_availableProbes[ui->listProbes->currentRow()];
    done(1);
}

void SelectProbeDialog::on_btnClose_clicked() {
    done(0);
}
