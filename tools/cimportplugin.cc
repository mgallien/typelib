#include "cimportplugin.hh"
#include "lang/cimport/import.hh"

#include <iostream>

#include "utilmm/configfile/configset.hh"
#include "registry.hh"

using namespace std;
using namespace boost;
using utilmm::config_set;
using Typelib::Registry;

CImportPlugin::CImportPlugin()
    : Plugin("C", "import") {}

list<string> CImportPlugin::getOptions() const
{
    static const char* arguments[] = 
    { ":include,I=string|include search path",
      ":define,D=string|Define this symbol" };
    return list<string>(arguments, arguments + 2);
}

bool CImportPlugin::apply(const OptionList& remaining, const config_set& options, Registry& registry)
{
    if (remaining.empty())
    {
        cerr << "No file found on command line. Aborting" << endl;
        return false;
    }
    string const file(remaining.front());

    try
    {
        CImport importer;
        if (! importer.load(file, options, registry))
            return false;

        return true;
    }
    catch(Typelib::RegistryException& e)
    {
        cerr << "Error in type management: " << e.toString() << endl;
        return false;
    }
    catch(std::exception& e)
    {
        cerr << "Error parsing file " << file << ":\n\t"
            << typeid(e).name() << endl;
        return false;
    }
}
