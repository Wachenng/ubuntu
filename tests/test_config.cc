#include "sylar/config.h"
#include "sylar/log.h"
#include <yaml-cpp/yaml.h>

sylar::ConfigVar<int>::ptr g_int_val_config = sylar::Config::Lookup("system.port", (int)8080, "system port");
sylar::ConfigVar<float>::ptr g_float_val_config = sylar::Config::Lookup("system.value", (float)10.2f, "system value");

void test_boost() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_val_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_val_config->toString();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_float_val_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_float_val_config->toString();
}

void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;

    } else if(node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << " NULL - " << node.Type() << " - " << level;

    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/ubuntu/server/bin/conf/log.yml");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
    print_yaml(root, 0);
}


void test_config() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_val_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_val_config->toString();

    YAML::Node root = YAML::LoadFile("/home/ubuntu/server/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_val_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_val_config->toString();
}

int main(int argc, char** argv) {
    // test_boost();
    // test_yaml();
    test_config();
    return 0;
}