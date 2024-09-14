
#pragma once

#include "result.h"
#include <QHash>
#include <QString>
#include <QVector>
#include <memory>
#include <optional>


class TypeBase;
class TypeChildInfo;

class IScope {
public:
    typedef std::shared_ptr<IScope> p;
    virtual QString scopeName() = 0;
    virtual p parentScope() = 0;
    virtual QString fullyQualifiedScopeName() = 0;
};

class TypeScopeBase : public IScope {
public:
    virtual QString fullyQualifiedScopeName() override {
        auto _parentScope = parentScope();
        QString ret;
        while (_parentScope.get() != nullptr) {
            ret += _parentScope->scopeName();
            ret += "::";
            _parentScope = _parentScope->parentScope();
        }
        ret += scopeName();
        return ret;
    }
};

class TypeScopeNamespace : public TypeScopeBase, public std::enable_shared_from_this<TypeScopeNamespace> {
public:
    typedef std::shared_ptr<TypeScopeNamespace> p;
    TypeScopeNamespace(QString name) : m_parentScope(nullptr), m_namespaceName(name) {}
    TypeScopeNamespace(QString name, p parentScope) : m_parentScope(parentScope), m_namespaceName(name) {}

    virtual QString scopeName() { return m_namespaceName; }
    virtual IScope::p parentScope() { return m_parentScope; }

private:
    QString m_namespaceName;
    IScope::p m_parentScope;
};

class IType {
public:
    IType() = default;

    enum class Kind {
        Unsupported,
        // Primitive Types
        Uint8,
        Sint8,
        Char = Sint8, // For convenience
        Uint16,
        Sint16,
        Uint32,
        Sint32,
        Uint64,
        Sint64,
        Float32,
        Float64,

        Structure,
        Class = Structure, // Who knows if we're going to use it
        Union,

        Enumeration,
    };
    enum Flags {
        NoFlags = 0,
        Export = 1,      // Usually comes with Anonymous flag, means that the members within can be accessed unqualified
        Anonymous = 2,   // Anonymous structure/union/class/enum etc
        Modified = 4,    // This is a modified type based on another type, is an instance of TypeModified
        Declaration = 8, // This type object represents an incomplete type (type declaration)
        HasVirtualInheritance = 16, // This structure contains virtual inheritance hierachy (propagates to derivatives)
        Exported = 32,              // This structural type has exported attribute (DWARF5)
        NamedByTypedef = 64,        // This structural type's type name was given based on an typedef alias
    };
    Q_DECLARE_FLAGS(TypeFlags, Flags);

    enum class Operation { Deref, Index };

    virtual Kind kind() = 0;
    virtual QString displayName() = 0;
    virtual QString fullyQualifiedName() = 0;
    virtual bool expandable() = 0;
    virtual Result<QVector<TypeChildInfo>, std::nullptr_t> getChildren() = 0;
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) = 0;
    virtual Result<std::shared_ptr<IType>, std::nullptr_t> getOperated(Operation op) = 0;
    virtual bool isTypedef() { return false; }
    virtual TypeFlags flags() { return NoFlags; }
    virtual size_t getSizeof() = 0;

    typedef std::shared_ptr<IType> p;
};

class TypeBase : public IType, public std::enable_shared_from_this<TypeBase> {
public:
    virtual Kind kind() override { return m_kind; }
    virtual QString fullyQualifiedName() override {
        auto parentScope = m_parentScope;
        QString ret;
        while (parentScope.get() != nullptr) {
            ret += parentScope->scopeName();
            ret += "::";
        }
        ret += displayName();
        return ret;
    }
    virtual TypeFlags flags() override { return TypeFlags({NoFlags}); }

    typedef std::shared_ptr<TypeBase> p;

protected:
    Kind m_kind;
    std::shared_ptr<IScope> m_parentScope;
};

struct TypeChildInfo {
    typedef uint32_t offset_t;
    enum Flags {
        NoFlags = 0,
        FromInheritance = 1,           ///< This member is synthesized from inherited subclass
        FromAnonymousSubstructure = 2, ///< This member is synthesized from an anonymous substructure
        AnonymousSubstructure = 4,     ///< This member is an anonymous substructure
    };
    QString name;
    IType::p type;
    std::optional<offset_t> byteOffset;
    uint8_t flags;
    uint8_t bitOffset; // This and the following only applies to bitfield members
    uint8_t bitWidth;  // For bitfields: First offset (byteOffset) bytes, then offset (bitOffset) bits, and the bitfield
                       // occupies (bitWidth) bits
};

// Base type of unsupported types, like non-power-of-2 length of integers, function pointers, etc.
class TypeUnsupported : public IType, public std::enable_shared_from_this<TypeUnsupported> {
    virtual Kind kind() override { return Kind::Unsupported; }
    virtual QString displayName() override { return "<UnsupportedType>"; }
    virtual QString fullyQualifiedName() override { return displayName(); }
    virtual bool expandable() override { return false; }
    virtual Result<QVector<TypeChildInfo>, std::nullptr_t> getChildren() override { return Err(nullptr); }
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) override { return Err(nullptr); };
    virtual Result<IType::p, std::nullptr_t> getOperated(Operation op) override { return Err(nullptr); };
    virtual size_t getSizeof() override { return 0; }
};

class TypePrimitive : public TypeBase {
public:
    TypePrimitive(Kind kind) { m_kind = kind; }
    virtual QString displayName() override {
        switch (m_kind) {
            case IType::Kind::Uint8: return "char";
            case IType::Kind::Sint8: return "unsigned char";
            case IType::Kind::Uint16: return "short";
            case IType::Kind::Sint16: return "unsigned short";
            case IType::Kind::Uint32: return "int";
            case IType::Kind::Sint32: return "unsigned int";
            case IType::Kind::Uint64: return "long long";
            case IType::Kind::Sint64: return "unsigned long long";
            case IType::Kind::Float32: return "float";
            case IType::Kind::Float64: return "double";
            default: Q_UNREACHABLE(); break;
        }
        return "";
    }
    virtual bool expandable() override { return false; }
    virtual Result<QVector<TypeChildInfo>, std::nullptr_t> getChildren() override { return Err(nullptr); }
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) override { return Err(nullptr); };
    virtual Result<IType::p, std::nullptr_t> getOperated(Operation op) override { return Err(nullptr); };
    virtual size_t getSizeof() override {
        switch (m_kind) {
            case IType::Kind::Uint8:
            case IType::Kind::Sint8: return 1;
            case IType::Kind::Uint16:
            case IType::Kind::Sint16: return 2;
            case IType::Kind::Uint32:
            case IType::Kind::Sint32:
            case IType::Kind::Float32: return 4;
            case IType::Kind::Uint64:
            case IType::Kind::Sint64:
            case IType::Kind::Float64: return 8; break;
            default: Q_UNREACHABLE(); break;
        }
        return 0;
    }
};

class TypeStructure : public TypeBase, public TypeScopeBase {
public:
    // We act lazy here because correctly initializing a TypeStructure in ctor is hard
    friend class SymbolBackend;

    TypeStructure(Kind kind) { m_kind = kind; }
    virtual QString displayName() override { return m_typeName; }
    virtual bool expandable() override { return !m_children.isEmpty(); }
    virtual Result<QVector<TypeChildInfo>, std::nullptr_t> getChildren() override { return Ok(m_children); }
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) override { return Err(nullptr); };
    virtual Result<IType::p, std::nullptr_t> getOperated(Operation op) override { return Err(nullptr); };
    virtual size_t getSizeof() override { return m_byteSize; }
    virtual TypeFlags flags() override {
        return TypeFlags({m_anonymous ? Anonymous : NoFlags, m_exported ? Exported : NoFlags,
                          m_declaration ? Declaration : NoFlags,
                          m_hasVirtualInheritance ? HasVirtualInheritance : NoFlags,
                          m_namedByTypedef ? NamedByTypedef : NoFlags}) |
               TypeBase::flags();
    }

    virtual QString scopeName() override { return m_typeName; }
    virtual IScope::p parentScope() override { return m_parentScope; }

private:
    QString m_typeName;
    IScope::p m_parentScope;
    QVector<TypeChildInfo> m_children;
    QVector<IType::p> m_inheritance;
    QHash<QString, int> m_childrenMap;
    size_t m_byteSize = 0;
    bool m_anonymous = false;
    bool m_exported = false;
    bool m_declaration = false;
    bool m_hasVirtualInheritance = false;
    bool m_namedByTypedef = false;

    void addMember(TypeChildInfo &child) { m_children.append(child); }
};

class TypeModified : public TypeBase {
public:
    friend class SymbolBackend;
    enum class Modifier {
        Array,
        Pointer,
    };
    static constexpr int arrayExpansionLimit = 50;
    TypeModified(IType::p baseType, Modifier mod, std::optional<size_t> additional)
        : m_baseType(baseType), m_mod(mod), m_additional(additional) {
        m_kind = baseType->kind();
        // DWARF gives us upper bound, not capacity. We store capacity instead.
        if (mod == Modifier::Array && m_additional.has_value()) {
            m_additional.value()++;
        }
    }
    virtual QString displayName() override {
        if (m_mod == Modifier::Pointer && m_baseType->kind() == Kind::Unsupported) {
            return "void*"; // For all pointers to unsupported types, we return void*
        }
        // Inserting modifiers properly. This is magic. Recommended readings:
        // http://unixwiz.net/techtips/reading-cdecl.html
        // Because C declarations are so messed up, we'd better traverse the entire modification chain to properly apply
        // modifiers. Enjoy!
        std::shared_ptr<TypeModified> lastType,
            currentType = std::static_pointer_cast<TypeModified>(shared_from_this());
        QString ret;
        while (currentType.get() != nullptr) {
            if (currentType->m_mod == Modifier::Array && lastType && lastType->m_mod == Modifier::Pointer &&
                !ret.isEmpty()) {
                // When last modifier is a pointer, we must enclose displayName with parenthesis for correct precedence
                ret = '(' + ret + ')';
            }
            switch (currentType->m_mod) {
                case Modifier::Array: ret.append(currentType->modifierString()); break;
                case Modifier::Pointer: ret.prepend(currentType->modifierString()); break;
                default: break;
            }
            // Switch to next depth
            lastType = currentType;
            currentType = std::dynamic_pointer_cast<TypeModified>(currentType->m_baseType);
        }
        // Put base type text in
        ret = (lastType->m_baseType ? lastType->m_baseType->displayName() : "<ErrorType>") + ret;
        return ret;
    }
    virtual bool expandable() override {
        // Base type must not be unsupported (function pointer is not expandable, for example)
        // If the modified type is an array, element count must be less than 50 (to avoid performance degredation)
        return (m_baseType->kind() != Kind::Unsupported) &&
               (m_additional.value_or(std::numeric_limits<size_t>::max()) < arrayExpansionLimit);
    }
    virtual Result<QVector<TypeChildInfo>, std::nullptr_t> getChildren() override {
        if (m_children.isEmpty()) {
            if (generateChildrenList()) {
                return Ok(m_children);
            } else {
                return Err(nullptr);
            }
        }
        return Ok(m_children);
    }
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) override {
        // clang-format off
        // Future contributors will hate this one-liner
        if (struct { bool ok; uint64_t n; } x; m_mod == Modifier::Array && (x.n = childName.toULongLong(&x.ok), x.ok)) {
            return Ok(TypeChildInfo{
                QString("[%1]").arg(childName), m_baseType, m_baseType->getSizeof() * x.n, 0, 0, 0
            });
            // clang-format on
        } else if (m_children.isEmpty() && !generateChildrenList()) {
            return Err(nullptr);
        } else if (auto child = m_childrenMap.find(childName); child != m_childrenMap.end()) {
            return Ok(m_children[*child]);
        }
        return Err(nullptr);
    }
    virtual Result<IType::p, std::nullptr_t> getOperated(Operation op) override { return Ok(m_baseType); }
    virtual size_t getSizeof() override {
        switch (m_mod) {
            case Modifier::Array: return m_baseType->getSizeof() * m_additional.value_or(0);
            case Modifier::Pointer: return m_additional.value_or(4); // Default to 32 bit machine
        }
        Q_UNREACHABLE();
        return 0;
    }
    virtual TypeFlags flags() override { return TypeFlags(Modified) | m_baseType->flags(); }

    Modifier modifier() { return m_mod; }

private:
    bool generateChildrenList() {
        if (m_baseType->kind() == IType::Kind::Unsupported)
            return false;
        switch (m_mod) {
            case Modifier::Array:
                for (int i = 0; i < std::min(m_additional.value_or(0), size_t(arrayExpansionLimit)); i++) {
                    TypeChildInfo childInfo;
                    childInfo.type = m_baseType;
                    childInfo.name = QString("[%1]").arg(i);
                    childInfo.flags = 0;
                    childInfo.bitOffset = 0;
                    childInfo.bitWidth = 0;
                    childInfo.byteOffset = i * m_baseType->getSizeof();
                    m_children.append(childInfo);
                    m_childrenMap[childInfo.name] = m_children.size() - 1;
                }
                return true;
            case Modifier::Pointer: {
                TypeChildInfo childInfo;
                childInfo.type = m_baseType;
                childInfo.name = "*";
                childInfo.flags = 0;
                childInfo.bitOffset = 0;
                childInfo.bitWidth = 0;
                childInfo.byteOffset.reset(); // Dereferencing will make offset undetermined
                m_children.append(childInfo);
                m_childrenMap[childInfo.name] = 0;
                return true;
            }
        }
        return false;
    }

private:
    IType::p m_baseType;
    Modifier m_mod;
    std::optional<size_t> m_additional;
    QVector<TypeChildInfo> m_children;
    QHash<QString, int> m_childrenMap;
    QString modifierString() {
        switch (m_mod) {
            case Modifier::Array:
                if (m_additional.value_or(0) != 0)
                    return QString("[%1]").arg(m_additional.value());
                else
                    return "[]";
            case Modifier::Pointer: return "*";
        }
        Q_UNREACHABLE();
        return 0;
    }
};
