
#pragma once

#include "libdwarf.h"
#include "typerepresentation.h"
#include <QDebug>
#include <QEventLoop>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QProgressDialog>
#include <QString>
#include <QTimer>
#include <optional>
#include <result.h>


template <typename T>
using Option = std::optional<T>;
struct ITypePtrBox;

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
        NoSymbolLoaded,
        DwarfApiFailure,
        InvalidParameter,
        ExplainTypeFailed,
        DwarfFailedToGetAddressSize,
        DwarfReferredDieNotFound,
        DwarfDieTypeInvalid,
        DwarfDieFormatInvalid,
        DwarfAttrNotConstant,
    };

    enum class VariableIconType {
        Unknown,
        Integer,
        FloatingPoint,
        Boolean,
        Structure,
        Pointer,
        Array,
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

    /**
     * @brief This is a list of reserved special CU indicies that appear only in m_typeMap keys, used for internal type
     * representations and conveniences
     *
     */
    enum class ReservedCu {
        // This reserved virtual CU holds all the defined primitive types, offsets = IType::Kind enum values.
        InternalPrimitiveTypes = -1,

        // This reserved virtual CU has a single type instance "TypeUnsupported" at offset = 0.
        InternalUnsupportedTypes = -2,
    };

    struct TypeSpec {};

    /**
     * @brief Because a type DIE ref can cross CU boundary (this is disgusting) we must include the target CU index
     */
    struct DieRef {
        DieRef() = default;
        DieRef(int cu, Dwarf_Off off) : cuIndex(cu), dieOffset(off) {}
        DieRef(QPair<int, Dwarf_Off> pair) : cuIndex(pair.first), dieOffset(pair.second) {}
        DieRef(const DieRef &) = default;
        DieRef &operator=(const DieRef &) = default;
        bool operator==(const DieRef &r) const { return cuIndex == r.cuIndex && dieOffset == r.dieOffset; }
        friend bool operator<(const DieRef &l, const DieRef &r) {
            return l.cuIndex < r.cuIndex || (l.cuIndex == r.cuIndex && l.dieOffset < r.dieOffset);
        }
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
        VariableNode() : iconType(VariableIconType::Unknown), expandable(false), bitOffset(0), bitSize(0) {}
        QString displayName;
        QString displayTypeName;
        VariableIconType iconType;
        bool expandable;
        std::optional<TypeChildInfo::offset_t> address;
        uint8_t bitOffset, bitSize;
        IType::p typeObj;
    };

    struct ExpandNodeResult {
        QVector<VariableNode> subNodeDetails;
    };

    /**
     * @brief Convert SymbolBackend error enumeration values to error strings.
     *
     * @param error Error enumeration values.
     * @return QString Error message.
     */
    static QString errorString(Error error);

    /**
     * @brief Checks if a symbol file is currently loaded.
     */
    bool isSymbolFileLoaded() { return m_loadSucceeded; }

    /**
     * @brief Switch the currently selected symbol file. Usually this is called when user selects a new symbol file,
     *        along with several other methods to refresh the entire workspace state.
     *
     * @param symbolFileFullPath full path to symbol file
     * @return Whether the operation was successful
     */
    Result<void, Error> switchSymbolFile(QString symbolFileFullPath);

    /**
     * @brief Get the path of symbol file.
     *
     * @return On success: symbol file path. On failure: error code.
     */
    Result<QString, Error> getSymbolFilePath();

    /**
     * @brief Get the source file list for the current symbol file, with basic information about variables inside
     *
     * @return Result<QStringList, Error>
     */
    Result<QStringList, Error> getSourceFileList();

    /**
     * @brief Get root variables of a source file
     *
     * @param sourceFilePath Source file path as returned by getSourceFileList
     * @return Result<QList<VariableNode>, Error>
     */
    Result<QList<VariableNode>, Error> getVariableOfSourceFile(QString sourceFilePath);

    // TODO:
    Result<ExpandNodeResult, Error> getVariableChildren(std::optional<TypeChildInfo::offset_t> parentOffset,
                                                        IType::p typeObj);

    /**
     * @brief Get the root scope of the symbol file.
     */
    IScope::p getRootScope() { return std::static_pointer_cast<IScope>(m_rootNamespace); }

    /**
     * @brief Get a root level scope by its name.
     */
    Option<IScope::p> getScope(QString scopeName);

    /**
     * @brief Get a root level type by its name.
     */
    Option<IType::p> getType(QString typeName);

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

    struct DwarfCuData {
        QString Name;                      ///< CU file name
        QString CompileDir;                ///< Directory in which it was compiled from
        QString Producer;                  ///< Compiler info string
        Dwarf_Die CuDie;                   ///< DIE associated with CU
        Dwarf_Off CuDieOff;                ///< CU Base Offset
        QMap<uint64_t, Dwarf_Die> TopDies; ///< Top level DIEs of CU DIE, indexed with offset
        QMap<uint64_t, Dwarf_Die> Dies;    ///< All (including those unused) DIEs of CU DIE, indexed with offset
        Dwarf_Die die(Dwarf_Off offset) { return Dies.value(offset /* + CuDieOff*/); }
        bool hasDie(Dwarf_Off offset) { return Dies.contains(offset /* + CuDieOff*/); }

        // Cached entries
        struct TypeDieDetails {
            QString displayName;
            Dwarf_Half dwarfTag;
            DieRef arraySubrangeElementTypeDie; ///< Exclusively for subranges
            bool expandable;
        };
        QMap<uint64_t, TypeDieDetails> CachedTypes;        ///< DIE offset -> type details cache
        QMap<uint64_t, VariableEntry::p> ExposedVariables; ///< DIE CU-Local offset -> Global/static variable entry
    };

    void addDie(int cu, Dwarf_Off cuOffset, Dwarf_Die die, DieRef parentDieRef);

    /**
     * @brief Create the primitive types, unsupported types' representation in m_typeMap.
     *
     */
    void createInternalTypes();

    // Functions to fetch internal types
    IType::p getPrimitive(IType::Kind kind);
    IType::p getUnsupported();

    bool isCuQualifiedSourceFile(const DwarfCuData &cuData);

    /**
     * @brief Convert global DIE offset to CU-local offset.
     * @return On success: CU index and CU-local offset. On failure: nothing.
     */
    Option<QPair<int, Dwarf_Off>> dieOffsetGlobalToCuBased(Dwarf_Off globalOffset);

    /**
     * @brief Deref of DW_FORM_ref*, either CU local or global. Args are same as dwarf_formref
     * @return On success: a pair of <CuIndex, CuLocalOffset>. On failure: Last libdwarf API call result
     */
    Result<QPair<int, Dwarf_Off>, int> anyDeref(Dwarf_Attribute dw_attr, Dwarf_Bool *dw_is_info, Dwarf_Error *dw_err);

    /**
     * @brief Get the internal type representation for a type DIE. If the DIE is unresolved, resolution will be
     * attempted internally recursively.
     *
     * @param typeDie DIE reference.
     * @return On success: IType ptr. On failure: error code.
     */
    Result<IType::p, Error> derefTypeDie(DieRef typeDie);

    /**
     * @brief Should only be called by derefTypeDie when a type DIE is not found in m_typeMap (unresolved).
     * Please note that a failed resolution will not be stored into the map.
     *
     * @param typeDie DIE reference.
     * @return On success: TypeBase ptr. On failure: error code.
     */
    Result<IType::p, Error> resolveTypeDie(DieRef typeDie);

    Result<TypeScopeNamespace::p, Error> buildNamespaceDie(DieRef nsDie);

    static void writeTypeInfoToVariableNode(VariableNode &node, IType::p type);
    static VariableIconType dwarfTagToIconType(Dwarf_Half tag);
    static bool isANestableType(Dwarf_Half tag);
    static void propagateOffset(std::optional<TypeChildInfo::offset_t> &base, std::optional<TypeChildInfo::offset_t>);

    // Custom extended DWARF manipulators
    union DwarfFormedInt {
        Dwarf_Unsigned u;
        Dwarf_Signed s;
    };
    Option<DieRef> DwarfDieToDieRef(Dwarf_Die die); ///< Convert a DIE to a DieRef
    Result<Dwarf_Off, int> DwarfCuDataOffsetFromDie(Dwarf_Die die,
                                                    Dwarf_Error *); ///< Get CU data offset for the owner CU of this DIE
    Result<DwarfFormedInt, int> DwarfFormInt(Dwarf_Attribute attr);
    Result<DwarfFormedInt, Error> DwarfFormConstant(Dwarf_Attribute attr); ///< Added for Keil member location

private:
    QProgressDialog m_progressDialog;
    QString m_symbolFileFullPath;
    bool m_loadSucceeded; // Whether the last symbol load has succeeded

    // libdwarf context
    Dwarf_Debug m_dwarfDbg;
    QVector<DwarfCuData> m_cus;              ///< Index in this vec is used to find the specific CU
    QMap<Dwarf_Off, int> m_cuOffsetMap;      ///< (CuBaseOffset -> CuVectorIndex) mapping
    QMap<DieRef, IType::p> m_typeMap;        ///< (CuOffset -> IType) mapping, for entire file
    QMap<DieRef, IScope::p> m_scopeMap;      ///< (CuOffset -> IScope) mapping, for entire file
    QMultiHash<QString, int> m_qualifiedCus; ///< (Source Files -> CUs that contain global variables) mapping
    QStringList m_qualifiedSourceFiles;      ///< Source files that contain global variables

    TypeScopeNamespace::p m_rootNamespace;
    Dwarf_Error m_err;
    Dwarf_Half m_machineWordSize; ///< In bytes

    // Resolution-local data
    QList<DieRef> m_resolutionTypeDies;
    QList<DieRef> m_resolutionNamespaceDies;
    QMap<DieRef, DieRef> m_resolutionNestedVariableCandidates; // (VariableDie, ParentDie) Parent DIE -> IScope
    QList<DieRef> m_resolutionTopLevelVariableDies;
};

// Because you cannot put a shared_ptr into QVariant (and you cannot extend shared_ptr to implement that)
// You'll need a dedicated struct to do that, which is stupid
struct ITypePtrBox {
    operator QVariant() { return QVariant::fromValue(*this); }
    IType::p p;
};

Q_DECLARE_METATYPE(SymbolBackend::DieRef);
Q_DECLARE_METATYPE(ITypePtrBox);
QDebug operator<<(QDebug debug, const SymbolBackend::DieRef &c);
QDebug operator<<(QDebug debug, const std::optional<TypeChildInfo::offset_t> &c);

// Hasher for DieRef
inline uint qHash(const SymbolBackend::DieRef &key, uint seed = 0) {
    return qHash(key.cuIndex, seed) ^ qHash(key.dieOffset, seed);
}
