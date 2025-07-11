
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
    psprobe_session_close(m_session);
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

ReadResult PSProbeSession::readMemory8(uint64_t address, size_t count) {
    QByteArray ret(count, Qt::Initialization::Uninitialized);
    auto code = psprobe_session_read_memory_8(m_session, m_coreSelected, address, count, ret.data());
    if (code) {
        return Err(Error{QObject::tr("Read memory error: Backend code: %1").arg(code), true,
                         ErrorClass::UnspecifiedBackendError});
    } else {
        return Ok(ret);
    }
}

ReadResult PSProbeSession::readMemory16(uint64_t address, size_t count) {
    QByteArray ret(count * 2, Qt::Initialization::Uninitialized);
    auto code = psprobe_session_read_memory_16(m_session, m_coreSelected, address, count, ret.data());
    if (code) {
        return Err(Error{QObject::tr("Read memory error: Backend code: %1").arg(code), true,
                         ErrorClass::UnspecifiedBackendError});
    } else {
        return Ok(ret);
    }
}

ReadResult PSProbeSession::readMemory32(uint64_t address, size_t count) {
    QByteArray ret(count * 4, Qt::Initialization::Uninitialized);
    auto code = psprobe_session_read_memory_32(m_session, m_coreSelected, address, count, ret.data());
    if (code) {
        return Err(Error{QObject::tr("Read memory error: Backend code: %1").arg(code), true,
                         ErrorClass::UnspecifiedBackendError});
    } else {
        return Ok(ret);
    }
}

ReadResult PSProbeSession::readMemory64(uint64_t address, size_t count) {
    QByteArray ret(count * 8, Qt::Initialization::Uninitialized);
    auto code = psprobe_session_read_memory_64(m_session, m_coreSelected, address, count, ret.data());
    if (code) {
        return Err(Error{QObject::tr("Read memory error: Backend code: %1").arg(code), true,
                         ErrorClass::UnspecifiedBackendError});
    } else {
        return Ok(ret);
    }
}

Result<void, Error> PSProbeSession::writeMemory8(uint64_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::writeMemory16(uint64_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::writeMemory32(uint64_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::writeMemory64(uint64_t address, const QByteArray &data) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

Result<void, Error> PSProbeSession::setReadScatterGatherList(const QVector<ScatterGatherEntry> &list) {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

ReadResult PSProbeSession::readScatterGather() {
    return Err(Error{"Not implemented", true, ErrorClass::UnspecifiedBackendError});
}

} // namespace probelib
