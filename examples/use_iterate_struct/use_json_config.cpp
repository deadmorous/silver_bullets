// This is an advanced example of usage of the ConfigLoader class.

#include "use_json_config.hpp"

#include "silver_bullets/iterate_struct/ConfigLoader.hpp"
#include "silver_bullets/iterate_struct/reportParameterOrigin.h"

#include "silver_bullets/templatize/resolve_template_args.hpp"

#include <iostream>
#include <boost/program_options.hpp>

using namespace std;
using namespace silver_bullets::iterate_struct;

enum class ParamType { One, Two, Vector };

SILVER_BULLETS_BEGIN_DEFINE_ENUM_NAMES(ParamType)
    { ParamType::One, "one" },
    { ParamType::Two, "two" },
    { ParamType::Vector, "vector" }
SILVER_BULLETS_END_DEFINE_ENUM_NAMES()

struct OneParam {
    int p = 0;
};
SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(OneParam, p);

struct TwoParam {
    int p1 = 1;
    int p2 = 2;
    JsonValue json;
};
SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(TwoParam, p1, p2, json);

struct VectorParam {
    vector<int> p;
};
SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(VectorParam, p);

template <ParamType paramType> struct ParamDispatch;
template <ParamType paramType> using ParamDispatch_t = typename ParamDispatch<paramType>::type;
template <> struct ParamDispatch<ParamType::One> { using type = OneParam; };
template <> struct ParamDispatch<ParamType::Two> { using type = TwoParam; };
template <> struct ParamDispatch<ParamType::Vector> { using type = VectorParam; };

template <ParamType paramType>
struct Foo
{
    ParamType type = paramType;
    ParamDispatch_t<paramType> parameters;
};
SILVER_BULLETS_DESCRIBE_TEMPLATE_STRUCTURE_FIELDS(((ParamType, ParamType)), Foo, type, parameters)

template<typename T, typename = void>
struct ValueChecker
{
    void operator()(const rapidjson::Value&) const
    {
        cout << "==== NOT REALLY CHECKING A VALUE" << endl;
    }
};

template<typename T>
struct ValueChecker<T, enable_if_t<has_iterate_struct_helper_v<T>>>
{
    void operator()(const rapidjson::Value& node) const
    {
        cout << "==== CHECKING STRUCT: " << typeid (T).name() << endl;
        if (!node.IsObject()) {
            cout << "==== ERROR: JSON Value is not an object" << endl;
            return;
        }

        set<string> names;
        T tmp;
        for_each(tmp, [&](auto&, const char* name) {
            names.insert(name);
        });

        auto o = node.GetObject();
        for (auto it=o.MemberBegin(); it!=o.MemberEnd(); ++it) {
            auto name = it->name.GetString();
            if (names.count(name) == 0)
                cout << "==== ERROR: JSON object contains an extra name: " << name << endl;
            names.erase(name);
        }

        for(auto& name: names) {
            cout << "==== WARNING: JSON object does not contain name: " << name << endl;
        }
        cout << "==== DONE" << endl;
    }
};

template<typename T>
void check_value(const rapidjson::Value& node)
{
    ValueChecker<T>{}(node);
}

class ConfigValidator
{
public:
    template<class T>
    inline void enterNode(const rapidjson::Value& /*node*/, const char *name)
    {
        cout << pad() << "ENTER NODE " << name
             << ": " << typeid (T).name() << endl;
        ++m_level;
    }

    template<class T>
    inline void exitNode(const rapidjson::Value& /*node*/, const char *name)
    {
        --m_level;
        cout << pad() << "EXIT NODE " << name << endl;
    }

    template<class T>
    inline void enterValue(const rapidjson::Value& node)
    {
        cout << pad() << "ENTER VALUE: "
             << typeid(T).name() << endl;
        check_value<T>(node);
    }

    template<class T>
    inline void exitValue(const rapidjson::Value& /*node*/)
    {
        cout << pad() << "EXIT VALUE" << endl;
    }

private:
    unsigned int m_level = 0;
    string pad() const {
        return string(m_level*4, ' ');
    }
};

template <ParamType paramType>
void useConfig(const ConfigLoader& configLoader)
{
    auto foo = configLoader.value<Foo<paramType>>(ConfigValidator{});
    cout << "Loaded data from config:" << endl;
    write_json_doc(cout, to_json_doc(foo));
    cout << "\n\nParameter origin report:" << endl;
    reportParameterOrigin(cout, foo, configLoader.configParamOrigin());
}

struct useConfigCaller {
    template<ParamType paramType>
    void operator()(const ConfigLoader& configLoader) const {
        useConfig<paramType>(configLoader);
    }
};

struct CmdLineOptions
{
    string config = "config_example.json";
    boost::optional<string> type;
};

void use_json_config(int argc, char *argv[])
{
    cout << "\n********** use_json_config() **********" << endl;

    namespace po = boost::program_options;
    po::options_description po_generic("Gerneric options");
    po_generic.add_options()
            ("help,h", "produce help message");
    po::options_description po_basic("Main options");
    auto po_value = [](auto& x) {
        return boost::program_options::value(&x);
    };
    CmdLineOptions cmdLineOptions;
    po_basic.add_options()
            ("config", po_value(cmdLineOptions.config), "Config file name")
            ("type", po_value(cmdLineOptions.type), "Type");

    po::variables_map vm;
    auto po_alloptions = po::options_description().add(po_generic).add(po_basic);
    po::store(po::command_line_parser(argc, argv)
              .options(po_alloptions).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << po_alloptions << "\n";
        return;
    }

    auto fileName = ConfigLoader::findConfigFile(cmdLineOptions.config);
    if (fileName.empty()) {
        cerr << "Failed to find configuration file '" << cmdLineOptions.config << "'" << endl
             << "Default config: " << endl;
        write_json_doc(cerr, to_json_doc(Foo<ParamType::Two>()));
        cerr << endl;
    }
    else {
        ConfigLoader configLoader(fileName, [&cmdLineOptions](ConfigLoader& cl) {
            string origin = "command line";
            if (cmdLineOptions.type)
                cl.setOptionalString(cmdLineOptions.type, "/type", origin);
        });
        auto paramType = configLoader.valueAt<ParamType>("/type", ParamType::Two);
        silver_bullets::resolve_template_args<
                integer_sequence<ParamType, ParamType::One, ParamType::Two, ParamType::Vector>>
                (make_tuple(paramType), useConfigCaller(), configLoader);
    }
}
