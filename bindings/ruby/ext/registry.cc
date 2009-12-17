#include "typelib.hh"

#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/exporter.hh>
#include <typelib/registryiterator.hh>
#include <utilmm/configfile/configset.hh>

using namespace Typelib;
using utilmm::config_set;
using std::string;
using namespace typelib_ruby;
using cxx2rb::RbRegistry;


namespace typelib_ruby {
    VALUE cRegistry = Qnil;
    VALUE eNotFound = Qnil;
}

/***********************************************************************************
 *
 * Wrapping of the Registry class
 *
 */


static 
void registry_free(void* ptr) { delete reinterpret_cast<RbRegistry*>(ptr); }
static
void registry_mark(void* ptr)
{
    using cxx2rb::WrapperMap;
    WrapperMap const& wrappers = reinterpret_cast<RbRegistry const*>(ptr)->wrappers;
    for (WrapperMap::const_iterator it = wrappers.begin(); it != wrappers.end(); ++it)
	rb_gc_mark(it->second);
}

static
VALUE registry_wrap(VALUE klass, Registry* registry)
{
    return Data_Wrap_Struct(klass, registry_mark, registry_free, new RbRegistry(registry));
}

static
VALUE registry_alloc(VALUE klass)
{
    Registry* registry = new Registry;
    return registry_wrap(klass, registry);
}


static
VALUE registry_includes_p(VALUE self, VALUE name)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const* type = registry.get( StringValuePtr(name) );
    return type ? Qtrue : Qfalse;
}

static
VALUE registry_do_get(VALUE self, VALUE name)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const* type = registry.get( StringValuePtr(name) );

    if (! type)
        rb_raise(eNotFound, "there is no type in this registry with the name '%s'", StringValuePtr(name));
    return cxx2rb::type_wrap(*type, self);
}

static
VALUE registry_do_build(VALUE self, VALUE name)
{
    Registry& registry = rb2cxx::object<Registry>(self);
    Type const* type = registry.build( StringValuePtr(name) );

    if (! type) 
        rb_raise(eNotFound, "cannot find %s in registry", StringValuePtr(name));
    return cxx2rb::type_wrap(*type, self);
}


/* call-seq:
 *  alias(new_name, name)	    => self
 *
 * Make +new_name+ refer to the type named +name+
 */
static
VALUE registry_alias(VALUE self, VALUE name, VALUE aliased)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    try { 
	registry.alias(StringValuePtr(aliased), StringValuePtr(name)); 
	return self;
    } catch(BadName) {
        rb_raise(rb_eArgError, "invalid type name %s", StringValuePtr(name));
    } catch(Undefined) {
        rb_raise(eNotFound, "there is not type in this registry with the name '%s'", StringValuePtr(aliased));
    }
}

static
void setup_configset_from_option_array(config_set& config, VALUE options)
{
    for (int i = 0; i < RARRAY_LEN(options); ++i)
    {
	VALUE entry = RARRAY_PTR(options)[i];
	VALUE k = RARRAY_PTR(entry)[0];
	VALUE v = RARRAY_PTR(entry)[1];

	if ( rb_obj_is_kind_of(v, rb_cArray) )
	{
            VALUE first_el = rb_ary_entry(v, 0);
            if (rb_obj_is_kind_of(first_el, rb_cArray))
            {
                // We are building recursive config sets
                for (int j = 0; j < RARRAY_LEN(v); ++j)
                {
                    config_set* child = new config_set;
                    setup_configset_from_option_array(*child, rb_ary_entry(v, j));
                    config.insert(StringValuePtr(k), child);
                }
            }
            else
            {
                for (int j = 0; j < RARRAY_LEN(v); ++j)
                {
                    VALUE value = rb_ary_entry(v, j);
                    config.insert(StringValuePtr(k), StringValuePtr(value));
                }
            }
	}
	else if (TYPE(v) == T_TRUE || TYPE(v) == T_FALSE)
	    config.set(StringValuePtr(k), RTEST(v) ? "true" : "false");
	else
	    config.set(StringValuePtr(k), StringValuePtr(v));
    }
}

/* Private method to import a given file in the registry
 * We expect Registry#import to format the arguments before calling
 * do_import
 */
static
VALUE registry_import(VALUE self, VALUE file, VALUE kind, VALUE merge, VALUE options)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    config_set config;
    setup_configset_from_option_array(config, options);
    
    std::string error_string;
    try { 
	if (RTEST(merge))
	{
	    Registry temp;
	    PluginManager::load(StringValuePtr(kind), StringValuePtr(file), config, temp); 
	    registry.merge(temp);
	}
	else
	    PluginManager::load(StringValuePtr(kind), StringValuePtr(file), config, registry); 
	return Qnil;
    }
    catch(std::runtime_error const& e) { error_string = e.what(); }
    catch(boost::bad_lexical_cast e)   { error_string = e.what(); }

    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}

/* Private method called by Registry#export. This latter method is supposed
 * to format the +options+ argument from a Ruby hash into an array suitable
 * for setup_configset_from_option_array
 */
static
VALUE registry_export(VALUE self, VALUE kind, VALUE options)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    config_set config;
    setup_configset_from_option_array(config, options);
    
    string error_message;
    try {
	std::string exported = PluginManager::save(StringValuePtr(kind), config, registry);
	return rb_str_new(exported.c_str(), exported.length());
    }
    catch (std::runtime_error e) { error_message = e.what(); }
    rb_raise(rb_eRuntimeError, error_message.c_str());
}


/*
 * call-seq:
 *   merge(other_registry) => self
 *
 * Move all types found in +other_registry+ into +self+
 */
static
VALUE registry_merge(VALUE self, VALUE rb_merged)
{
    std::string error_string;

    Registry& registry = rb2cxx::object<Registry>(self);
    Registry& merged   = rb2cxx::object<Registry>(rb_merged);
    try { 
	registry.merge(merged);
	return self;
    }
    catch(std::runtime_error& e) { error_string = e.what(); }
    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}


/*
 * call-seq:
 *   resize(new_sizes) => nil
 *
 * Changes the size of some types, while modifying the types that depend on
 * them to keep the registry consistent.
 */
static
VALUE registry_resize(VALUE self, VALUE new_sizes)
{
    Registry& registry = rb2cxx::object<Registry>(self);

    std::map<std::string, size_t> sizes;
    size_t map_size   = RARRAY_LEN(new_sizes);
    VALUE* map_values = RARRAY_PTR(new_sizes);
    for (size_t i = 0; i < map_size; ++i)
    {
        VALUE* pair = RARRAY_PTR(map_values[i]);
        sizes.insert(std::make_pair(
                StringValuePtr(pair[0]),
                NUM2INT(pair[1])));
    }
    try { 
	registry.resize(sizes);
	return Qnil;
    }
    catch(std::runtime_error& e) {
        rb_raise(rb_eRuntimeError, "%s", e.what());
    }
}

/* call-seq:
 *  minimal(auto_types) => registry
 *
 * Returns the minimal registry needed to define all types that are in +self+
 * but not in +auto_types+
 */
static
VALUE registry_minimal(VALUE self, VALUE rb_auto)
{
    std::string error_string;

    Registry& registry   = rb2cxx::object<Registry>(self);
    Registry& auto_types = rb2cxx::object<Registry>(rb_auto);
    try { 
	Registry* result = registry.minimal(auto_types);
        return registry_wrap(cRegistry, result);
    }
    catch(std::runtime_error e) { error_string = e.what(); }
    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}


/*
 * each_type(include_alias = false) { |type| ... }
 * each_type(include_alias = true)  { |name, type| ... }
 *
 * Iterates on the types found in this registry. If include_alias is true, also
 * yield the aliased types.
 */
static
VALUE registry_each_type(int argc, VALUE* argv, VALUE self)
{
    VALUE rb_include_alias;
    rb_scan_args(argc, argv, "01", &rb_include_alias);
    bool include_alias = RTEST(rb_include_alias);

    Registry& registry = rb2cxx::object<Registry>(self);
    std::string error_string;

    try 
    {
	RegistryIterator it = registry.begin();
	RegistryIterator end = registry.end();
	if (include_alias)
	{
	    for (; it != end; ++it)
	    {
		std::string const& type_name = it.getName();
		VALUE rb_type_name = rb_str_new(type_name.c_str(), type_name.length());
		rb_yield_values(2, rb_type_name, cxx2rb::type_wrap(*it, self));
	    }
	}
	else
	{
	    for (; it != end; ++it)
	    {
		if (!it.isAlias())
		    rb_yield(cxx2rb::type_wrap(*it, self));
	    }
	}

	return self;
    }
    catch(std::runtime_error e) { error_string = e.what(); }
    rb_raise(rb_eRuntimeError, "%s", error_string.c_str());
}

/* call-seq:
 *  Registry.from_xml => string
 * 
 * Build a registry from a string, which is formatted as Typelib's own XML
 * format.  See also #export, #import and #to_xml
 */
static
VALUE registry_from_xml(VALUE mod, VALUE xml)
{
    VALUE rb_registry = rb_funcall(cRegistry, rb_intern("new"), 0);
    Registry& registry = rb2cxx::object<Registry>(rb_registry);

    std::istringstream istream(StringValuePtr(xml));
    config_set config;
    try { PluginManager::load("tlb", istream, config, registry); }
    catch(std::runtime_error e)
    { rb_raise(rb_eArgError, "cannot load xml: %s", e.what()); }
    catch(boost::bad_lexical_cast e)
    { rb_raise(rb_eArgError, "cannot load xml: %s", e.what()); }

    return rb_registry;
}

/*
 * call-seq:
 *  registry.define_container(kind, element_type) => new_type
 *
 * Defines a new container instance with the given container kind and element
 * type. +element_type+ is a type object, i.e. a subclass of Type, that has to
 * be part of +registry+ (obtained through registry.get or registry.build). If
 * +element_type+ comes from another registry object, the method raises
 * ArgumentError.
 *
 * Moreover, the method raises NotFound if +kind+ is not a known container name.
 */
static VALUE registry_define_container(VALUE registry, VALUE kind, VALUE element)
{
    Registry& reg = rb2cxx::object<Registry>(registry);
    Type const& element_type(rb2cxx::object<Type>(element));
    // Check that +reg+ contains +element_type+
    if (!reg.isIncluded(element_type))
        rb_raise(rb_eArgError, "the given type object comes from a different type registry");

    try {
        Container const& new_type = Container::createContainer(reg, StringValuePtr(kind), element_type);
        return cxx2rb::type_wrap(new_type, registry);
    } catch(Typelib::UnknownContainer) {
        rb_raise(eNotFound, "%s is not a known container type", StringValuePtr(kind));
    }
}

void typelib_ruby::Typelib_init_registry()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    cRegistry = rb_define_class_under(mTypelib, "Registry", rb_cObject);
    eNotFound = rb_define_class_under(mTypelib, "NotFound", rb_eRuntimeError);
    rb_define_alloc_func(cRegistry, registry_alloc);
    rb_define_method(cRegistry, "get", RUBY_METHOD_FUNC(registry_do_get), 1);
    rb_define_method(cRegistry, "build", RUBY_METHOD_FUNC(registry_do_build), 1);
    rb_define_method(cRegistry, "each_type", RUBY_METHOD_FUNC(registry_each_type), -1);
    // do_import is called by the Ruby-defined import, which formats the 
    // option hash (if there is one), and can detect the import type by extension
    rb_define_method(cRegistry, "do_import", RUBY_METHOD_FUNC(registry_import), 4);
    rb_define_method(cRegistry, "do_export", RUBY_METHOD_FUNC(registry_export), 2);
    rb_define_singleton_method(cRegistry, "from_xml", RUBY_METHOD_FUNC(registry_from_xml), 1);
    rb_define_method(cRegistry, "alias", RUBY_METHOD_FUNC(registry_alias), 2);
    rb_define_method(cRegistry, "merge", RUBY_METHOD_FUNC(registry_merge), 1);
    rb_define_method(cRegistry, "minimal", RUBY_METHOD_FUNC(registry_minimal), 1);
    rb_define_method(cRegistry, "includes?", RUBY_METHOD_FUNC(registry_includes_p), 1);
    rb_define_method(cRegistry, "do_resize", RUBY_METHOD_FUNC(registry_resize), 1);

    rb_define_method(cRegistry, "define_container", RUBY_METHOD_FUNC(registry_define_container), 2);
}

