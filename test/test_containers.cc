#include <boost/test/auto_unit_test.hpp>
#include <vector>

#include <test/testsuite.hh>
#include <utilmm/configfile/configset.hh>
#include <utilmm/stringtools.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include "test_cimport.1"
#include <string.h>
using namespace Typelib;
using namespace std;
using utilmm::split;
using utilmm::join;

BOOST_AUTO_TEST_CASE( test_vector_assumptions )
{
    // It is expected that ->size() depends on the type of the vector elements,
    // but that the size can be offset by using the ratio of sizeof()
    {
        vector<int32_t> values;
        values.resize(10);
        BOOST_REQUIRE_EQUAL(10, values.size());
        BOOST_REQUIRE_EQUAL(5, reinterpret_cast< vector<int64_t>& >(values).size());
    }
    {
        vector< vector<double> > values;
        values.resize(10);
        BOOST_REQUIRE_EQUAL(10, values.size());
        BOOST_REQUIRE_EQUAL(10 * sizeof(vector<double>), reinterpret_cast< vector<int8_t>& >(values).size());
    }

    // It is expected that some byte-copies operations are valid with vectors
    // The test do not really do anything by itself. Result should be checked
    // with Valgrind
    {
        vector< vector<int32_t> > values;
        values.resize(10);
        for (int i = 0; i < 10; ++i)
            values[i].resize(i + 1);

        memcpy(&values[5], &values[6], sizeof(vector<int32_t>) * 4);

        {
            vector<uint8_t>* raw_values = reinterpret_cast< vector<uint8_t>* >(&values);
            raw_values->resize(raw_values->size() - sizeof(vector<int32_t>));
        }

        for (int i = 0; i < 5; ++i)
            BOOST_REQUIRE_EQUAL(i + 1, values[i].size());
        for (int i = 5; i < 9; ++i)
            BOOST_REQUIRE_EQUAL(i + 2, values[i].size());
    }
}

static void import_test_types(Registry& registry)
{
    static const char* test_file = TEST_DATA_PATH("test_cimport.h");

    utilmm::config_set config;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    config.set("include", TEST_DATA_PATH(".."));
    config.set("define", "GOOD");
    BOOST_REQUIRE_NO_THROW( importer->load(test_file, config, registry) );
}

struct AssertValueVisit : public ValueVisitor
{
    vector<int32_t> values;
    bool visit_(int32_t& v)
    {
        values.push_back(v);
        return true;
    }
};

BOOST_AUTO_TEST_CASE( test_vector_getElementCount )
{
    Registry registry;
    import_test_types(registry);
    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("B"));

    std::vector<B> vector;
    vector.resize(10);
    BOOST_REQUIRE_EQUAL(10, container.getElementCount(&vector));
}

BOOST_AUTO_TEST_CASE( test_vector_init_destroy )
{
    Registry registry;
    import_test_types(registry);
    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("B"));
    BOOST_REQUIRE_EQUAL(Type::Container, container.getCategory());

    void* v_memory = malloc(sizeof(std::vector<B>));
    container.init(v_memory);
    BOOST_REQUIRE_EQUAL(0, container.getElementCount(v_memory));
    container.destroy(v_memory);
    free(v_memory);
}

BOOST_AUTO_TEST_CASE( test_vector_insert )
{
    Registry registry;
    import_test_types(registry);
    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("int"));

    std::vector<int> vector;
    for (int value = 0; value < 10; ++value)
    {
        container.insert(&vector, Value(&value, container.getIndirection()));
        BOOST_REQUIRE_EQUAL(value + 1, vector.size());

        for (int i = 0; i < value; ++i)
            BOOST_REQUIRE_EQUAL(i, vector[i]);
    }
}

BOOST_AUTO_TEST_CASE( test_vector_erase )
{
    Registry registry;
    import_test_types(registry);
    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("int"));

    std::vector<int> vector;
    for (int value = 0; value < 10; ++value)
        vector.push_back(value);

    int value = 20;
    BOOST_REQUIRE(!container.erase(&vector, Value(&value, container.getIndirection())));
    value = 5;
    BOOST_REQUIRE(container.erase(&vector, Value(&value, container.getIndirection())));

    BOOST_REQUIRE_EQUAL(9, vector.size());
    for (int i = 0; i < 5; ++i)
        BOOST_REQUIRE_EQUAL(i, vector[i]);
    for (int i = 5; i < 9; ++i)
        BOOST_REQUIRE_EQUAL(i + 1, vector[i]);
}

bool test_delete_if_pred(Value v)
{
    int* value = reinterpret_cast<int*>(v.getData());
    return (*value > 3) && (*value < 6);
}
BOOST_AUTO_TEST_CASE( test_vector_delete_if )
{
    Registry registry;
    import_test_types(registry);
    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("int"));

    std::vector<int> vector;
    for (int value = 0; value < 10; ++value)
        vector.push_back(value);

    container.delete_if(&vector, test_delete_if_pred);

    BOOST_REQUIRE_EQUAL(8, vector.size());
    for (int i = 0; i < 4; ++i)
        BOOST_REQUIRE_EQUAL(i, vector[i]);
    for (int i = 4; i < 8; ++i)
        BOOST_REQUIRE_EQUAL(i + 2, vector[i]);
}

BOOST_AUTO_TEST_CASE( test_vector_visit )
{ 
    Registry registry;
    import_test_types(registry);

    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("int32_t"));

    std::vector<int32_t> v;
    v.resize(10);
    for (int i = 0; i < 10; ++i)
        v[i] = i;
    BOOST_REQUIRE_EQUAL(10, container.getElementCount(&v));

    // Try visiting the value
    Value value(&v, container);
    AssertValueVisit visitor;
    visitor.apply(value);

    BOOST_REQUIRE_EQUAL(10, visitor.values.size());
    for (int i = 0; i < 10; ++i)
        BOOST_REQUIRE_EQUAL(i, visitor.values[i]);
}

