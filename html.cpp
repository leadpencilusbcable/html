#include <iostream>
#include <set>
#include <algorithm>
#include <string.h>

#include "html.hpp"

using namespace html;

const std::set<std::string> self_closing = {
    "area",
    "base",
    "br",
    "col",
    "embed",
    "hr",
    "img",
    "input",
    "link",
    "meta",
    "param",
    "source",
    "track",
    "wbr",
};

element::element(std::string name): name(name) {}

std::vector<element>& element::get_children()
{
    return children;
}

void element::add_child(element ele)
{
    children.push_back(ele);
}

void element::add_inner_text(std::string text)
{
    inner_strings[children.size() + inner_strings.size()] = text;
}

std::unordered_map<std::string, std::string> element::get_attributes() const
{
    return attributes;
}

std::string* element::get_attribute(std::string attr)
{
    if(attributes.find(attr) != attributes.end()){;
        return &attributes[attr];
    }

    return nullptr;
}

void element::set_attribute(std::string attr, std::string val)
{
    attributes[attr] = val;
}

void search(std::vector<element*>& elements, element* ele, std::function<bool (element*)>& predicate, unsigned short max_count = 0, unsigned short count = 0)
{
    for(element& child : ele->get_children()){
        if(predicate(&child)){
            elements.push_back(&child);

            count++;
        };

        if(max_count != 0 && count == max_count)
            return;

        search(elements, &child, predicate, max_count, count);
    }
}

std::vector<element*> element::search_children(std::function<bool (element*)>& predicate, unsigned short max_count)
{
    std::vector<element*> elements;
    search(elements, this, predicate, max_count);

    return elements;
}

element* element::get_child_by_id(const std::string& name)
{
    std::function<bool (element*)> predicate = [name](html::element* ele){
        std::string* val = ele->get_attribute(std::string("id"));

        if(val != nullptr)
            return name == *val;

        return false;
    };

    std::vector<element*> result = search_children(predicate, 1);

    if(result.size() == 1)
        return result[0];

    return nullptr;
}

std::string element::element_string(element* ele, unsigned short indent_size, unsigned short indent = 0)
{
    std::string ret(indent, ' ');
    ret.append("<" + ele->name);

    for(const auto& [key, val] : ele->attributes){
        ret.append(" ");
        ret.append(key);
        ret.append("=\"");
        ret.append(val);
        ret.append("\"");
    }

    if(self_closing.find(ele->name) != self_closing.end() && ele->children.size() == 0){
        ret.append("/>");
        return ret;
    }

    ret.append(">\n");

    unsigned int counter = 0;

    for(size_t i = 0; i < ele->children.size() + ele->inner_strings.size(); i++){
        if(ele->inner_strings.count(i)){
            ret.append(std::string(indent + indent_size, ' ') + ele->inner_strings[i] + "\n");
            continue;
        }

        ret.append(element_string(&(ele->children[counter]), indent_size, indent + indent_size) + "\n");

        counter++;
    }

    ret.append(std::string(indent, ' '));
    ret.append("</" + ele->name + ">");

    return ret;
}

std::string element::to_string(unsigned short indent_size)
{
    return element_string(this, indent_size);
}
