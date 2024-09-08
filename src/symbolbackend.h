
#pragma once

#include "gdbcontainer.h"
#include "libdwarf.h"
#include <QEventLoop>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QProgressDialog>
#include <QString>
#include <QTimer>
#include <result.h>

/**
 * @brief SymbolBacked uses GdbContainer under the hood to provide all the necessary symbol information
 *        for data acquisition. Because it only has to evaluate the expressions very infrequently, it
 *        is designed to run with no concurrency (although GdbContainer can do so).
 *
 *        SymbolBackend also has to handle timeouts and GDB crashes. It will pop a dialog to let the user
 *        know of the situation, and, the recover may be unsuccessful. So others calling its methods should
 *        prepare for an unsuccessful API call.
 *
 *        GDB usually should reply very quickly, so the APIs here are designed to be synchronous.
 *
 *        SymbolBackend can only set GDB executable once. You are expected to construct it with a GDB executable
 *        path most of the time, and if it's OOBE, you should ask user about it and set it immediately. It then
 *        will start GDB automatically (or if it's constructed with a GDB exec, done so in the constructor).
 *        If the user wants to change GDB used, by design the application must be restarted.
 */
class SymbolBackend final : public QObject {
    Q_OBJECT

public:
    explicit SymbolBackend(QObject *parent = nullptr);
    ~SymbolBackend();

    /**
     * @brief Possible error values of SymbolBackend API calls.
     */
    enum class Error {
        NoError,
        GdbNotStarted,
        GdbResponseTimeout,
        GdbStartupFail,
        GdbCommandExecutionError,
        UnexpectedGdbConsoleOutput,
        UnsupportedCppIdentifier,
        LibdwarfApiFailure,
        InvalidParameter,
        ExplainTypeFailed
    };


    enum class VariableIconType {
        Integer,
        FloatingPoint,
        Boolean,
        Structure,
        Pointer,
        Array,
        Unknown,
    };

    enum class ElementaryTypes {
        Uint8,
        Sint8,
        Uint16,
        Sint16,
        Uint32,
        Sint32,
        Uint64,
        Sint64,
        Float32,
        Float64,
        Unsupported,
    };

    /**
     * @brief Expression generation is done by symbol panel. When symbol panel generate expression, it gets the variable
     * name represented by the currently selected node, then rewinds up to the root (CU), in this process, the parent
     * nodes are checked individually, their relations to the children nodes are checked, expression is generated from
     * inside out. This enumeration describes the parent-child relation used in this process.
     */
    enum class ExprGenRelation {
        Deref,  ///< Parent is a pointer and child is a dereferenced item
        Index,  ///< Parent is a pointer/array and child is the result of indexing with square brackets
        Member, ///< Parent is a structure/union/class and child is a member of it
    };

    struct TypeSpec {};

    /**
     * @brief Because a type DIE ref can cross CU boundary (this is disgusting) we must include the target CU index
     */
    struct TypeDieRef {
        TypeDieRef() = default;
        TypeDieRef(int cu, Dwarf_Off off) : cuIndex(cu), dieOffset(off) {}
        TypeDieRef(QPair<int, Dwarf_Off> pair) : cuIndex(pair.first), dieOffset(pair.second) {}
        TypeDieRef(const TypeDieRef &) = default;
        TypeDieRef &operator=(const TypeDieRef &) = default;
        operator QVariant() { return QVariant::fromValue(*this); }
        int cuIndex;
        Dwarf_Off dieOffset;
    };

    struct VariableInfo {
        QString name;
        QString typeName;
        QString explainedType;
        QString expression;
        unsigned int childrenCount;
    };

    struct SourceFile {
        QString path;
        uint32_t cuIndex; ///< CU Index when asking SymbolBackend about this CU's stuff
    };

    struct VariableNode {
        QString displayName;
        QString displayTypeName;
        VariableIconType iconType;
        bool expandable;
        void *address;
        TypeDieRef typeSpec;
    };

    struct ExpandNodeResult {
        QVector<VariableNode> subNodeDetails;
        Dwarf_Half tag;
    };

    /**
     * @brief Convert SymbolBackend error enumeration values to error strings.
     *
     * @param error Error enumeration values.
     * @return QString Error message.
     */
    static QString errorString(Error error);

    /**
     * @brief Switch the currently selected symbol file. Usually this is called when user selects a new symbol file,
     *        along with several other methods to refresh the entire workspace state.
     *
     * @param symbolFileFullPath full path to symbol file
     * @return Whether the operation was successful
     */
    Result<void, Error> switchSymbolFile(QString symbolFileFullPath);

    /**
     * @brief Get the source file list for the current symbol file, with basic information about variables inside
     *
     * @return Result<QList<SourceFile>, Error>
     */
    Result<QList<SourceFile>, Error> getSourceFileList();

    /**
     * @brief Get root variables of a source file
     *
     * @param cuIndex Compilation unit index returned with getSourceFileList
     * @return Result<QList<VariableNode>, Error>
     */
    Result<QList<VariableNode>, Error> getVariableOfSourceFile(uint32_t cuIndex);

    // TODO:
    Result<ExpandNodeResult, Error> getVariableChildren(QString varName, uint32_t cuIndex, TypeDieRef typeSpec);

private:
    /**
     * @brief DWARF Attribute list wrapper for ease of use.
     */
    class DwarfAttrList {
    public:
        DwarfAttrList(Dwarf_Debug dbg, Dwarf_Die die) : m_die(die), m_dbg(dbg), m_attrList(NULL), m_attrCount(0) {
            m_lastRet = dwarf_attrlist(die, &m_attrList, &m_attrCount, &m_err);
        }
        ~DwarfAttrList() {
            if (m_attrList && m_attrCount) {
                for (int i = 0; i < m_attrCount; i++) {
                    dwarf_dealloc_attribute(m_attrList[i]);
                }
                dwarf_dealloc(m_dbg, m_attrList, DW_DLA_LIST);
            }
        }
        // Get an attribute with a specific index in list
        Dwarf_Attribute operator[](Dwarf_Unsigned idx) const { return (idx >= m_attrCount) ? NULL : m_attrList[idx]; }
        // Get an attribute with a specific attribute number
        Dwarf_Attribute operator()(Dwarf_Half attrNumber) {
            if (!m_attrList || !m_attrCount)
                return NULL;
            for (Dwarf_Signed i = 0; i < count(); i++) {
                Dwarf_Half attrNum;
                m_lastRet = dwarf_whatattr(m_attrList[i], &attrNum, &m_err);
                if (attrNum == attrNumber)
                    return m_attrList[i];
            }
            return NULL;
        }
        // Move assignment
        DwarfAttrList &operator=(DwarfAttrList &&that) {
            if (this != &that) {
                // Same as dtor
                if (m_attrList && m_attrCount) {
                    for (int i = 0; i < m_attrCount; i++) {
                        dwarf_dealloc_attribute(m_attrList[i]);
                    }
                    dwarf_dealloc(m_dbg, m_attrList, DW_DLA_LIST);
                }
                // Copy everything
                m_lastRet = that.m_lastRet;
                m_dbg = that.m_dbg;
                m_die = that.m_die;
                m_err = that.m_err;
                m_attrCount = that.m_attrCount;
                m_attrList = that.m_attrList;
                // Invalidate the moved object
                that.m_attrList = NULL;
                that.m_attrCount = 0;
            }
            return *this;
        }
        bool has(Dwarf_Half attrNumber) {
            Dwarf_Bool ret;
            m_lastRet = dwarf_hasattr(m_die, attrNumber, &ret, &m_err);
            return ret;
        }
        int lastRet() const { return m_lastRet; }
        Dwarf_Signed count() const { return m_attrCount; }
        Dwarf_Error error() const { return m_err; }

    private:
        int m_lastRet;
        Dwarf_Debug m_dbg;
        Dwarf_Die m_die;
        Dwarf_Error m_err;
        Dwarf_Signed m_attrCount;
        Dwarf_Attribute *m_attrList;
    };

    struct TypeResolutionContext {
        TypeResolutionContext() : typedefTypeName(QString()), arrayElementTypeOff(0, 0) {}
        QString typedefTypeName;        ///< Outmost layer typedef defined type name
        TypeDieRef arrayElementTypeOff; ///< Array element type defined in DW_TAG_array_type
    };

    struct DwarfCuData {
        QString Name;                      ///< CU file name
        QString CompileDir;                ///< Directory in which it was compiled from
        QString Producer;                  ///< Compiler info string
        Dwarf_Die CuDie;                   ///< DIE associated with CU
        Dwarf_Off CuDieOff;                ///< CU Base Offset
        QMap<uint64_t, Dwarf_Die> TopDies; ///< Top level DIEs of CU DIE, indexed with offset
        QMap<uint64_t, Dwarf_Die> Dies;    ///< All DIEs of CU DIE, indexed with offset
        Dwarf_Die die(Dwarf_Off offset) { return Dies.value(offset /* + CuDieOff*/); }
        bool hasDie(Dwarf_Off offset) { return Dies.contains(offset /* + CuDieOff*/); }

        // Cached entries
        struct TypeDieDetails {
            QString displayName;
            Dwarf_Half dwarfTag;
            TypeDieRef arraySubrangeElementTypeDie; ///< Exclusively for subranges
            bool expandable;
        };
        QMap<uint64_t, TypeDieDetails> CachedTypes; ///< DIE offset -> type details cache
    };

    bool isCuQualifiedSourceFile(DwarfCuData &cuData);
    /**
     * @brief Deref of DW_FORM_ref*, either CU local or global. Args are same as dwarf_formref
     * @return On success: a pair of <CuIndex, CuLocalOffset>. On failure: Last libdwarf API call result
     */
    Result<QPair<int, Dwarf_Off>, int> anyDeref(Dwarf_Attribute dw_attr, Dwarf_Off *dw_out_off, Dwarf_Bool *dw_is_info,
                                                Dwarf_Error *dw_err);
    Result<DwarfCuData::TypeDieDetails, Error> getTypeDetails(uint32_t cuIndex, Dwarf_Off typeDie);
    Result<DwarfCuData::TypeDieDetails, Error> getTypeDetails(TypeDieRef typeDie);
    bool ensureTypeResolved(uint32_t cuIndex, Dwarf_Off typeDie);
    bool resolveTypeDetails(uint32_t cuIndex, Dwarf_Off typeDie, TypeResolutionContext &ctx);

    Result<ExpandNodeResult, Error> tryExpandType(uint32_t cuIndex, Dwarf_Off typeDie, TypeResolutionContext ctx,
                                                  QString varName);
    static VariableIconType dwarfTagToIconType(Dwarf_Half tag);

    // Custom extended DWARF manipulators
    int DwarfFormRefEx(Dwarf_Attribute dw_attr, Dwarf_Off *dw_out_off, Dwarf_Bool *dw_is_info, Dwarf_Error *dw_err);
    Result<Dwarf_Off, int> DwarfCuDataOffsetFromDie(Dwarf_Die die,
                                                    Dwarf_Error *); ///< Get CU data offset for the owner CU of this DIE

private:
    // GDB Shenanigans
    // GdbContainer m_gdb;
    QString m_symbolFileFullPath;
    bool m_gdbProperlySet;

    QEventLoop m_eventLoop;
    QTimer m_waitTimer;
    QProgressDialog m_progressDialog;

    gdbmi::Response m_lastGdbResponse;
    uint64_t m_expectedGdbResponseToken;

    // libdwarf to the rescue
    Dwarf_Debug m_dwarfDbg;
    QVector<DwarfCuData> m_cus;               ///< Index in this vec is used to find the specific CU
    QMap<Dwarf_Off, int> m_cuOffsetMap;       ///< (CuBaseOffset -> CuVectorIndex) mapping
    QList<SourceFile> m_qualifiedSourceFiles; ///< Source files considered "useful" in a sense that it contains globals
};

Q_DECLARE_METATYPE(SymbolBackend::TypeDieRef);
