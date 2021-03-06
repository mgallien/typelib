#ifndef __TYPELIB_TYPENAME_H__
#define __TYPELIB_TYPENAME_H__

#include <string>
#include <boost/tokenizer.hpp>

namespace Typelib
{
    typedef std::string::value_type   NamespaceMarkType;
    static const NamespaceMarkType    NamespaceMark = '/';
    static const NamespaceMarkType    TemplateMark  = '<';
    static const char*                NamespaceMarkString = "/";

    struct NameSeparator : public boost::char_separator<NamespaceMarkType>
    {
        NameSeparator()
            : boost::char_separator<NamespaceMarkType>(NamespaceMarkString, "") {}
    };
    typedef boost::tokenizer<NameSeparator> NameTokenizer;

    
    bool isAbsoluteName(const std::string& name);
    bool isValidNamespace(const std::string& name, bool absolute);
    bool isValidTypename(const std::string& name, bool absolute);
    bool isInNamespace(const std::string& type, const std::string& nspace, bool recursive = false);
    
    /** Returns +name+ as a namespace name, valid to build an absolute type
     * name by simple concatenation */
    std::string getNormalizedNamespace(const std::string& name);
    /** Removes the namespace part of \c full_name and returns it */
    std::string getTypename (const std::string& full_name);
    /** Returns the normalized namespace part of \c full_name. The returned
     * namespace name starts and ends with the namespace separator '/'. */
    std::string getNamespace(const std::string& full_name);
    /** Returns the relative part of +name+, relative to the given namespace.
     * The returned name does not have any leading namespace separator. */
    std::string getRelativeName(std::string const& full_name, std::string const& ns);
};

#endif


