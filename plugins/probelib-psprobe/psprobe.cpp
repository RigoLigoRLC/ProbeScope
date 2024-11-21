
#include "psprobe.h"
#include "psprobe/families.h"
#include "psprobe/probe.h"
#include "psprobe/session.h"


namespace probelib {
    PSProbe::PSProbe() {
        // Initialize device list
        m_devices.append("");
        size_t deviceId = 1;
        void *families;
        size_t count;
        psprobe_families_get(&families, &count);
        for (size_t i = 0; i < count; i++) {
            char *name;
            size_t nameLen;
            psprobe_families_get_name(families, i, &name, &nameLen);
            m_deviceCategories.append(DeviceCategory{QString::fromUtf8(name, nameLen)});
            auto &category = m_deviceCategories.back();
            size_t variantCount;
            psprobe_families_get_variant_count(families, i, &variantCount);
            for (size_t j = 0; j < variantCount; j++) {
                char *variantName;
                size_t variantNameLen;
                psprobe_families_get_variant_name(families, i, j, &variantName, &variantNameLen);
                auto deviceTuple = std::make_tuple(deviceId, QString::fromUtf8(variantName, variantNameLen));
                category.devices.append(deviceTuple);
                m_devices.append(std::get<1>(deviceTuple));
                deviceId++;
            }
        }
        psprobe_families_destroy(families);
    }

    QVector<IAvailableProbe::p> PSProbe::availableProbes() {
        // Clear old probes
        m_probesFetched.clear();

        // Fetch new probes
        size_t count;
        void *probes;
        psprobe_probe_list_get(&probes, &count);
        for (size_t i = 0; i < count; i++) {
            void *probe;
            psprobe_probe_list_get_probe(probes, i, &probe);
            m_probesFetched.append(std::make_shared<PSProbeAvailableProbe>(probe));
        }
        psprobe_probe_list_destroy(probes);

        // Convert to base interface
        QVector<IAvailableProbe::p> ret;
        foreach (auto probe, m_probesFetched) {
            ret.append(probe);
        }
        return ret;
    }

    Result<void, Error> PSProbe::selectProbe(IAvailableProbe::p probe) {
        // Close current probe
        if (m_currentProbe != nullptr) {
            psprobe_probe_close(m_currentProbe);
            m_currentProbe = nullptr;
        }
        // Take ExtProbeInfo
        auto probeInfo = std::dynamic_pointer_cast<PSProbeAvailableProbe>(probe)->m_rustProbe;
        // Get ExtProbe
        auto result = psprobe_probe_open(probeInfo, &m_currentProbe);
        if (result) {
            return Err(Error{tr("Failed to open probe"), true, ErrorClass::UnspecifiedBackendError});
        }
        return Ok();
    }

    const QVector<DeviceCategory> PSProbe::supportedDevices() {
        return m_deviceCategories;
    }


    Result<uint32_t, Error> PSProbe::setConnectionSpeed(uint32_t speed) {
        if (m_currentProbe == nullptr) {
            return Err(Error{tr("No probe selected"), true, ErrorClass::PreConfigurationFailure});
        }
        uint32_t actualSpeed = 0;
        auto result = psprobe_probe_set_connection_speed(m_currentProbe, speed);
        if (result) {
            return Err(Error{tr("Failed to set connection speed"), true, ErrorClass::PreConfigurationFailure});
        }
        return connectionSpeed();
    }

    Result<uint32_t, Error> PSProbe::connectionSpeed() {
        if (m_currentProbe == nullptr) {
            return Err(Error{tr("No probe selected"), true, ErrorClass::PreConfigurationFailure});
        }
        uint32_t speed = 0;
        auto result = psprobe_probe_get_connection_speed(m_currentProbe, &speed);
        if (result) {
            return Err(Error{tr("Failed to get connection speed"), true, ErrorClass::PreConfigurationFailure});
        }
        return Ok(speed);
    }

    Result<IProbeSession *, Error> PSProbe::connect(size_t deviceId) {
        if (m_currentProbe == nullptr) {
            return Err(Error{tr("No probe selected"), true, ErrorClass::BeginConnectionFailure});
        }

        if (m_connectionActive) {
            return Err(Error{tr("Connection already active"), true, ErrorClass::BeginConnectionFailure});
        }

        void *probeSession;
        auto result = psprobe_session_open(m_currentProbe, m_devices[deviceId].toUtf8().constData(),
                                           m_devices[deviceId].size(), &probeSession);
        if (result) {
            return Err(Error{tr("Failed to open session"), true, ErrorClass::BeginConnectionFailure});
        }

        m_connectionActive = true;
        return Ok((IProbeSession *) new PSProbeSession(probeSession, [this]() { unsetConnectionActiveFlag(); }));
    }
}