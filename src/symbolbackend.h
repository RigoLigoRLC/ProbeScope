
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
    explicit SymbolBackend(QString gdbPath, QObject *parent = nullptr);
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
        Dwarf_Off typeSpec;
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
     * @brief As described in the class brief, this method can only be called once when GDB was not set in constructor.
     *        One exception is that if the GDB path you've set before cannot start GDB, you have one more chance.
     * @note This method will start GDB automatically.
     * @note This method is also what used in constructor.
     *
     * @param gdbPath GDB executable path
     * @return true GDB executable path was set properly
     * @return false GDB executable path is already set and is already valid
     */
    bool setGdbExecutableLazy(QString gdbPath);

    /**
     * @brief Get the Gdb Terminal Device object. This QIODevice is only readable and is meant to display GDB activity.
     *
     * @return QIODevice* GDB Terminal device
     */
    GdbReadonlyDevice *getGdbTerminalDevice() { return m_gdb.getGdbTerminalDevice(); }

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

    /**
     * @brief Get the children count of an expression.
     *
     * @param expr Parent expression
     * @return On success: Children count. On fail: error code.
     */
    Result<uint64_t, Error> getVariableChildrenCount(QString expr);

    // TODO:
    Result<ExpandNodeResult, Error> getVariableChildren(QString varName, uint32_t cuIndex, Dwarf_Off typeSpec);

    /**
     * @brief Ask GDB about the real type info (traces typedefs) of a type name (with "whatis" command)
     *
     * @param typeName type name to be processed
     * @return Result<QString, Error>
     */
    Result<QString, Error> explainType(QString typeName);

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
        TypeResolutionContext() : typedefTypeName(QString()), arrayElementTypeOff(0) {}
        QString typedefTypeName;       ///< Outmost layer typedef defined type name
        Dwarf_Off arrayElementTypeOff; ///< Array element type defined in DW_TAG_array_type
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
            Dwarf_Off arraySubrangeElementTypeDie; ///< Exclusively for subranges
            bool expandable;
        };
        QMap<uint64_t, TypeDieDetails> CachedTypes; ///< DIE offset -> type details cache
    };

    Result<int, Error> warnUnstartedGdb(); // TODO: Unimplemented, called to check if GDB not started and warn user
    Result<gdbmi::Response, Error> retryableGdbCommand(QString command, int timeout = 5000);
    Result<gdbmi::Response, Error> waitGdbResponse(uint64_t token, int timeout = 5000);
    void recoverGdbCrash();

    bool isCuQualifiedSourceFile(DwarfCuData &cuData);
    Result<DwarfCuData::TypeDieDetails, Error> getTypeDetails(uint32_t cuIndex, Dwarf_Off typeDie);
    bool ensureTypeResolved(uint32_t cuIndex, Dwarf_Off typeDie);
    bool resolveTypeDetails(uint32_t cuIndex, Dwarf_Off typeDie, TypeResolutionContext &ctx);

    Result<ExpandNodeResult, Error> tryExpandType(uint32_t cuIndex, Dwarf_Off typeDie, TypeResolutionContext ctx,
                                                  QString varName);
    static VariableIconType dwarfTagToIconType(Dwarf_Half tag);

private slots:
    void gdbExited(bool normalExit, int exitCode);
    void gdbResponse(uint64_t token, const gdbmi::Response response);
    void gdbWaitTimedOut();

private:
    // GDB Shenanigans
    GdbContainer m_gdb;
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
    QList<SourceFile> m_qualifiedSourceFiles; ///< Source files considered "useful" in a sense that it contains globals
};
