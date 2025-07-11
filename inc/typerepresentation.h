
#pragma once

#include "result.h"
#include <QHash>
#include <QMap>
#include <QString>
#include <QVector>
#include <memory>
#include <optional>


class TypeChildInfo;
class IType;
class IScope;
class VariableEntry;

class IScope {
public:
    friend class SymbolBackend;
    typedef std::shared_ptr<IScope> p;
    virtual QString scopeName() = 0;
    virtual p parentScope() = 0;
    virtual QString fullyQualifiedScopeName() = 0;
    virtual std::shared_ptr<IType> getType(QString typeName) = 0;
    virtual std::shared_ptr<IScope> getSubScope(QString scopeName) = 0;
    virtual std::shared_ptr<VariableEntry> getVariable(QString name) = 0;
    virtual void mergeFrom(IScope::p source) = 0;

private:
    virtual void addType(std::shared_ptr<IType> type) = 0;
    virtual void addSubScope(std::shared_ptr<IScope> scope) = 0;
    virtual void addVariable(QString name, std::shared_ptr<VariableEntry> symbol) = 0;
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
    virtual IScope::p parentScope() = 0;
    virtual bool expandable() = 0;
    virtual Result<QVector<TypeChildInfo>, std::nullptr_t> getChildren() = 0;
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) = 0;
    virtual Result<std::shared_ptr<IType>, std::nullptr_t> getOperated(Operation op) = 0;
    virtual bool isTypedef() { return false; }
    virtual TypeFlags flags() { return NoFlags; }
    virtual size_t getSizeof() = 0;

    typedef std::shared_ptr<IType> p;

    static bool isSignedInteger(p type) {
        switch (type->kind()) {
            case Kind::Uint8:
            case Kind::Uint16:
            case Kind::Uint32:
            case Kind::Uint64: return false;
            case Kind::Sint8:
            case Kind::Sint16:
            case Kind::Sint32:
            case Kind::Sint64: return true;
            default: Q_ASSERT(false && "Not integer type");
        }
        return false;
    }
};

struct TypeChildInfo {
    typedef uint64_t offset_t;
    enum Flags {
        NoFlags = 0,
        FromInheritance = 1,           ///< This member is synthesized from inherited subclass
        FromAnonymousSubstructure = 2, ///< This member is synthesized from an anonymous substructure
        AnonymousSubstructure = 4,     ///< This member is an anonymous substructure
        Bitfield = 8,                  ///< This member have valid bitfield properties
    };
    QString name;
    IType::p type;
    std::optional<offset_t> byteOffset;
    uint8_t flags;
    uint8_t bitOffset; // This and the following only applies to bitfield members
    uint8_t bitWidth;  // For bitfields: First offset (byteOffset) bytes, then offset (bitOffset) bits, and the bitfield
                       // occupies (bitWidth) bits
};

struct VariableEntry {
    using p = std::shared_ptr<VariableEntry>;

    QString name;
    TypeChildInfo::offset_t offset;
    std::shared_ptr<IType> type;
    std::shared_ptr<IScope> scope;
};

class TypeScopeBase : public IScope {
public:
    friend class SymbolBackend;
    virtual QString fullyQualifiedScopeName() override {
        auto _parentScope = parentScope();
        QString ret;
        ret.prepend(scopeName());
        while (_parentScope.get() != nullptr) {
            if (_parentScope->scopeName().isEmpty())
                break; // Only Root namespace has an empty name
            ret.prepend("::");
            ret.prepend(_parentScope->scopeName());
            _parentScope = _parentScope->parentScope();
        }
        return ret;
    }
    virtual std::shared_ptr<IType> getType(QString typeName) override {
        if (auto type = m_types.find(typeName); type != m_types.end()) {
            return type.value();
        }
        return nullptr;
    }
    virtual std::shared_ptr<IScope> getSubScope(QString scopeName) override {
        if (auto scope = m_subScopes.find(scopeName); scope != m_subScopes.end()) {
            return scope.value();
        }
        return nullptr;
    }
    virtual VariableEntry::p getVariable(QString name) override {
        if (auto symbol = m_variables.find(name); symbol != m_variables.end()) {
            return symbol.value();
        }
        return {};
    }
    virtual void mergeFrom(IScope::p source) override {
        std::shared_ptr<TypeScopeBase> source2 = std::dynamic_pointer_cast<TypeScopeBase>(source);
        if (source2.get() == nullptr)
            return;
        for (auto &type : source2->m_types) {
            addType(type);
        }
        for (auto &scope : source2->m_subScopes) {
            addSubScope(scope);
        }
        for (auto it = source2->m_variables.begin(); it != source2->m_variables.end(); ++it) {
            addVariable(it.key(), it.value());
        }
    }

protected:
    virtual void addType(std::shared_ptr<IType> type) override { m_types[type->displayName()] = type; }
    virtual void addSubScope(std::shared_ptr<IScope> scope) override {
        // There can be scopes with same name. We need to transfer all stuff in the passed-in scope to the existing one
        if (auto it = m_subScopes.find(scope->scopeName()); it != m_subScopes.end()) {
            (*it)->mergeFrom(scope);
            return;
        }
        m_subScopes[scope->scopeName()] = scope;
        if (auto scope2 = std::dynamic_pointer_cast<TypeScopeBase>(scope); scope2.get() != nullptr) {
            scope2->reparent(sharedFinalFromThis());
        }
    }
    virtual void addVariable(QString name, VariableEntry::p symbol) override { m_variables[name] = symbol; }
    virtual void reparent(std::shared_ptr<IScope> newParent) = 0;
    virtual IScope::p sharedFinalFromThis() = 0;

    QHash<QString, std::shared_ptr<IType>> m_types;
    QHash<QString, std::shared_ptr<IScope>> m_subScopes;
    QHash<QString, VariableEntry::p> m_variables;
};

class TypeScopeNamespace : public TypeScopeBase, public std::enable_shared_from_this<TypeScopeNamespace> {
public:
    friend class SymbolBackend;
    typedef std::shared_ptr<TypeScopeNamespace> p;
    TypeScopeNamespace(QString name) : m_parentScope(nullptr), m_namespaceName(name) {}
    TypeScopeNamespace(QString name, p parentScope) : m_parentScope(parentScope), m_namespaceName(name) {}

    virtual QString scopeName() override { return m_namespaceName; }
    virtual IScope::p parentScope() override { return m_parentScope; }

protected:
    virtual void reparent(std::shared_ptr<IScope> newParent) override { m_parentScope = newParent; }
    virtual IScope::p sharedFinalFromThis() override { return shared_from_this(); }

private:
    QString m_namespaceName;
    IScope::p m_parentScope;
};

class TypeBase : public IType, public std::enable_shared_from_this<TypeBase> {
public:
    virtual Kind kind() override { return m_kind; }
    virtual QString fullyQualifiedName() override {
        auto parentScope = m_parentScope;
        QString ret;
        while (parentScope.get() != nullptr) {
            if (parentScope->scopeName().isEmpty())
                break; // Only Root namespace has an empty name
            ret += parentScope->scopeName();
            ret += "::";
            parentScope = parentScope->parentScope();
        }
        ret += displayName();
        return ret;
    }
    virtual IScope::p parentScope() override { return m_parentScope; }
    virtual TypeFlags flags() override { return TypeFlags({NoFlags}); }

    typedef std::shared_ptr<TypeBase> p;

protected:
    Kind m_kind;
    std::shared_ptr<IScope> m_parentScope;
};

// Base type of unsupported types, like non-power-of-2 length of integers, function pointers, etc.
class TypeUnsupported : public IType, public std::enable_shared_from_this<TypeUnsupported> {
    virtual Kind kind() override { return Kind::Unsupported; }
    virtual QString displayName() override { return "<UnsupportedType>"; }
    virtual QString fullyQualifiedName() override { return displayName(); }
    virtual IScope::p parentScope() override { return nullptr; }
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
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) override {
        if (auto it = m_childrenMap.find(childName); it == m_childrenMap.end()) {
            return Err(nullptr);
        } else {
            return Ok(m_children[it.value()]);
        }
    };
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

protected:
    virtual void reparent(std::shared_ptr<IScope> newParent) override { m_parentScope = newParent; }
    virtual IScope::p sharedFinalFromThis() override {
        return std::static_pointer_cast<TypeStructure>(shared_from_this());
    }

private:
    QString m_typeName;
    QVector<TypeChildInfo> m_children;
    QVector<IType::p> m_inheritance;
    QHash<QString, int> m_childrenMap;
    size_t m_byteSize = 0;
    bool m_anonymous = false;
    bool m_exported = false;
    bool m_declaration = false;
    bool m_hasVirtualInheritance = false;
    bool m_namedByTypedef = false;

    void addMember(TypeChildInfo &child) {
        m_children.append(child);
        m_childrenMap.insert(child.name, m_children.size() - 1);
    }
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

class TypeEnumeration : public TypeBase, public TypeScopeBase {
public:
    friend class SymbolBackend;

    TypeEnumeration(QString typeName, size_t byteSize, QMap<int64_t, QString> enumMap)
        : m_typeName(typeName), m_byteSize(byteSize), m_enumMap(enumMap) {}
    virtual Kind kind() override { return Kind::Enumeration; }
    virtual QString displayName() override { return m_typeName; }
    virtual bool expandable() override { return false; }
    virtual Result<QVector<TypeChildInfo>, std::nullptr_t> getChildren() override { return Err(nullptr); }
    virtual Result<TypeChildInfo, std::nullptr_t> getChild(QString childName) override { return Err(nullptr); };
    virtual Result<IType::p, std::nullptr_t> getOperated(Operation op) override { return Err(nullptr); };
    virtual size_t getSizeof() override { return m_byteSize; }
    virtual TypeFlags flags() override {
        return TypeFlags({m_typeName.isEmpty() ? Anonymous : NoFlags, m_declaration ? Declaration : NoFlags,
                          m_namedByTypedef ? NamedByTypedef : NoFlags}) |
               TypeBase::flags();
    }

    virtual QString scopeName() override { return m_typeName; }
    virtual IScope::p parentScope() override { return m_parentScope; }

protected:
    virtual void reparent(std::shared_ptr<IScope> newParent) override { m_parentScope = newParent; }
    virtual IScope::p sharedFinalFromThis() override {
        return std::static_pointer_cast<TypeStructure>(shared_from_this());
    }

private:
    QString m_typeName;
    QMap<int64_t, QString> m_enumMap;
    size_t m_byteSize = 0;
    bool m_declaration = false;
    bool m_namedByTypedef = false;
};
