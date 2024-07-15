#pragma once

#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

namespace html
{
    class element
    {
        private:
            std::unordered_map<std::string, std::string> attributes;
            std::vector<element> children;
            std::unordered_map<unsigned short, std::string> inner_strings;
            std::string element_string(element* ele, unsigned short indent_size, unsigned short indent);
        public:
            element(std::string name);

            std::string name;

            std::vector<element>& get_children();
            void add_child(element element);
            void add_inner_text(std::string text);
            std::unordered_map<std::string, std::string> get_attributes() const;
            std::string* get_attribute(std::string attr);
            void set_attribute(std::string attr, std::string val);
            std::vector<element*> search_children(std::function<bool (element*)>& predicate, unsigned short max_count = 0);
            element* get_child_by_id(const std::string& name);
            std::string to_string(unsigned short indent_size = 2);
    };
}
