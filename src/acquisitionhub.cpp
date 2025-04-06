
#include "acquisitionhub.h"
#include "probelibhost.h"

AcquisitionHub::AcquisitionHub(ProbeLibHost *probeLibHost, QObject *parent)
    : m_plh(probeLibHost), QObject(parent), m_acquisitionThread(acquisitionThread, this) {
    //
}

AcquisitionHub::~AcquisitionHub() {
    //
}

void AcquisitionHub::acquisitionThread(AcquisitionHub *self) {
    //
}
