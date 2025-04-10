
#include "psprobe/session.h"
#include "bridge.h"
#include "psprobe.h"

namespace probelib {

PSProbeSession::PSProbeSession(void *session, std::function<void()> onDisconnect) : m_disconnectCallback(onDisconnect) {
    m_session = session;

    // Initially select the first core
    auto cores = listCores();
    Q_ASSERT(listCores().isOk());
    Q_ASSERT(cores.unwrap().size() > 0);
    selectCore(cores.unwrap().at(0).coreId);
}

PSProbeSession::~PSProbeSession() {
    m_disconnectCallback();
}

Result<QVector<CoreDescriptor>, Error> PSProbeSession::listCores() {
    Q_ASSERT(m_session);
    QVector<CoreDescriptor> ret;

    void *core_info;
    size_t count;

    psprobe_core_info_get(m_session, &core_info, &count);
    for (size_t i = 0; i < count; i++) {
        size_t core_index;
        psprobe_ext_dyn_str core_desc;
        psprobe_core_info_get_entry(core_info, i, &core_index, &core_desc);
        // FIXME: this core_desc may be nullptr? Please check Rust side and C++ side and figure out what to do
        // ret.append(CoreDescriptor{core_index, QString() << core_desc});
        ret.append(CoreDescriptor{core_index, QString()});
    }
    psprobe_core_info_destroy(core_info);

    return Ok(ret);
}

Result<void, Error> PSProbeSession::selectCore(size_t core) {
    m_coreSelected = core;
    return Ok();
}

ReadResult PSProbeSession::readMemory8(size_t address, size_t count) {
    return Ok(QByteArray(1, 0));
    // return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

ReadResult PSProbeSession::readMemory16(size_t address, size_t count) {
    return Ok(QByteArray(2, 0));
    // return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

ReadResult PSProbeSession::readMemory32(size_t address, size_t count) {
    return Ok(QByteArray(4, 0));
    // return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

ReadResult PSProbeSession::readMemory64(size_t address, size_t count) {
    return Ok(QByteArray(8, 0));
    // return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::writeMemory8(size_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::writeMemory16(size_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::writeMemory32(size_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::writeMemory64(size_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::setReadScatterGatherList(const QVector<ScatterGatherEntry> &list) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

ReadResult PSProbeSession::readScatterGather() {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

} // namespace probelib
