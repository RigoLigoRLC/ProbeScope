
#pragma once

#include "probelib/iprobelib.h"
#include "ui_selectprobedialog.h"
#include <QDialog>


/**
 * @brief Used to select a ProbeLib and a probe.
 *
 * Provide it with a list of ProbeLibs, currect ProbeLib and current selected probe, call execWithState.
 * It will return true if a new probe was selected, false if the dialog was closed without selecting a new probe.
 * When true is returned, check other methods for the new ProbeLib and Probe. Try connect to the new probe.
 *
 */
class SelectProbeDialog : protected QDialog {
    Q_OBJECT
public:
    explicit SelectProbeDialog(QWidget *parent = nullptr);
    ~SelectProbeDialog();

    bool execWithState(QVector<probelib::IProbeLib *> availableProbeLibs, probelib::IProbeLib *currentProbeLib,
                       probelib::IAvailableProbe::p currentProbe);

    probelib::IProbeLib *selectedProbeLib() const { return m_selectedProbeLib; }
    probelib::IAvailableProbe::p selectedProbe() const { return m_selectedProbe; }

private:
    void selectProbeLib(probelib::IProbeLib *probeLib);
    void scanProbes(probelib::IProbeLib *probeLib, probelib::IAvailableProbe::p currentProbe);

private slots:
    void on_cmbProbeLib_currentIndexChanged(int index);
    void on_btnRescan_clicked();
    void on_btnSelect_clicked();
    void on_btnClose_clicked();

private:
    // Only kept until dialog is closed
    // Initial states
    QVector<probelib::IProbeLib *> m_availableProbeLibs;
    probelib::IProbeLib *m_currentProbeLib = nullptr;
    probelib::IAvailableProbe::p m_currentProbe;
    // Dynamic states constructed at runtime
    probelib::IProbeLib *m_selectedProbeLib = nullptr;
    probelib::IAvailableProbe::p m_selectedProbe;
    QVector<probelib::IAvailableProbe::p> m_availableProbes;

private:
    Ui::SelectProbeDialog *ui;
};
