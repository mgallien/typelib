#ifndef TYPELIB_EXPORT_IDL_HH
#define TYPELIB_EXPORT_IDL_HH

#include <typelib/exporter.hh>

/* Exports types or a whole registry in the CORBA Interface Definition Language (IDL).
 * This exporter accepts the following options:
 * * \c namespace_prefix: a namespace to prepend to the type namespace. Useful
 *      to have the normal C types coexist with the CORBA-generated types.
 * * \c namespace_suffix: a namespace to add to the types suffixes. Useful
 *      to have the normal C types coexist with the CORBA-generated types.
 * * \c blob_threshold: if set to a nonzero value, a type which is bigger than
 *      the given value will be exported as a sequence<octet>. Useful if
 *      performance is important, and if type safety is not an issue (for
 *      instance if the software using CORBA implements its own type safety
 *      mechanisms)
 */
class IDLExport : public Typelib::Exporter
{
    std::string m_namespace;
    std::string m_ns_prefix;
    std::string m_ns_suffix;
    std::string m_indent;
    int m_blob_threshold;
    bool m_opaque_as_any;

    std::set<std::string> m_selected_types;

    struct TypedefSpec
    {
        std::string name_space;
        std::string declaration;
    };
    typedef std::map<std::string, std::list<std::string> > TypedefMap;
    TypedefMap m_typedefs;

    void closeNamespaces(std::ostream& stream, int levels);
    void adaptNamespace(std::ostream& stream, std::string const& ns);
    void generateTypedefs(std::ostream& stream);

protected:
    /** Called by save to add data after saving all registry types */
    virtual void end  (std::ostream& stream, Typelib::Registry const& registry);

public:
    IDLExport();

    /** Returns the namespace, in typelib-format, in which a type in +type_ns+
     * should be exported. In particular, it takes into account m_ns_prefix and
     * m_ns_suffix
     */
    std::string getExportNamespace(std::string const& type_ns) const;
    /** Returns the name in which +type+ should be represented in the IDL file.
     * This converts base types into the corresponding IDL base types, and
     * removes "struct" and "union" from C-related names
     */
    static std::string getIDLTypename(Typelib::Type const& type);
    /** Returns the IDL type for the basic +type+
     */
    static std::string getIDLBaseType(Typelib::Numeric const& type);
    /** Returns the absolute name (including the full namespace) to reference
     * +type+ in the IDL file. If +type+ is in the current namespace, no
     * the type basename is returned instead.
     */
    std::string getIDLAbsoluteTypename(Typelib::Type const& type) const;
    /** Returns the absolute name (including the full namespace) to reference
     * +type+ in the IDL file. If +type+ is in current_namespace, the type
     * basename is returned instead.
     */
    std::string getIDLAbsoluteTypename(Typelib::Type const& type, std::string const& current_namespace) const;

    virtual void save
        ( std::ostream& stream
	, utilmm::config_set const& config
        , Typelib::Registry const& type);

    virtual void save
        ( std::ostream& stream
        , Typelib::RegistryIterator const& type);

    static void checkType(Typelib::Type const& type);
};

#endif

