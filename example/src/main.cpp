#include <iostream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <vector>
#include <chrono>
#include <string>

#include "../../html.hpp"
#include "./monster.hpp"
#include "../include/mongoose.h"
#include "../include/rapidjson/document.h"
#include "../include/rapidjson/stringbuffer.h"

std::vector<monster> monsters;

struct mg_str caps[2];

std::unordered_map<std::string, std::string> cache;

html::element construct_monster_table_body(std::vector<monster> table_monsters)
{
    html::element tbody("tbody");

    for(monster mon : table_monsters){
        html::element body_tr("tr");

        html::element name_td("td");
        name_td.add_inner_text(mon.name);

        html::element cr_td("td");
        cr_td.add_inner_text(mon.cr);

        html::element hp_td("td");
        hp_td.add_inner_text(std::to_string(mon.hp));

        html::element ac_td("td");
        ac_td.add_inner_text(std::to_string(mon.ac));

        html::element speed_td("td");
        speed_td.add_inner_text("0");

        html::element swim_speed_td("td");
        swim_speed_td.add_inner_text("0");

        html::element fly_speed_td("td");
        fly_speed_td.add_inner_text("0");

        body_tr.add_child(name_td);
        body_tr.add_child(cr_td);
        body_tr.add_child(hp_td);
        body_tr.add_child(ac_td);
        body_tr.add_child(speed_td);
        body_tr.add_child(swim_speed_td);
        body_tr.add_child(fly_speed_td);

        tbody.add_child(body_tr);
    }

    return tbody;
}

//taken from https://stackoverflow.com/a/24327749
std::vector<std::string> split(std::string const& original, char separator)
{
    std::vector<std::string> results;
    std::string::const_iterator start = original.begin();
    std::string::const_iterator end = original.end();
    std::string::const_iterator next = std::find(start, end, separator);

    while(next != end){
        results.push_back(std::string(start, next));
        start = next + 1;
        next = std::find(start, end, separator);
    }

    results.push_back(std::string(start, next));

    return results;
}

std::map<std::string, std::string> parse_query_params(const std::string& query_params)
{
    std::vector<std::string> query_param_strings = split(query_params, '&');

    std::map<std::string, std::string> query_param_map;

    for(const std::string& str : query_param_strings){
        std::vector<std::string> key_value = split(str, '=');

        query_param_map[key_value[0]] = key_value[1];
    }

    return query_param_map;
}

static void fn(struct mg_connection* c, int ev, void* ev_data)
{
    if(ev == MG_EV_HTTP_MSG){
        struct mg_http_message *hm = (struct mg_http_message*) ev_data;

        if(mg_strcmp(hm->uri, mg_str("/")) == 0){
            struct mg_http_serve_opts opts = {
                .mime_types = "html=text/html"
            };

            mg_http_serve_file(c, hm, "./public/pages/index.html", &opts);
        }

        else if(mg_strcmp(hm->uri, mg_str("/monsters")) == 0){
            //TODO: need to restrict cache keys so that malicious user cannot create infinite cache entries
            std::string cache_key = "";
            std::map<std::string, std::string> q_params;

            if(hm->query.len){
                cache_key = std::string(hm->query.buf).substr(0, hm->query.len);
                q_params = parse_query_params(cache_key);
            }

            auto cache_val = cache.find(cache_key);

            if(cache_val != cache.end()){
                std::string table_string = cache_val->second;

                mg_http_reply(c, 200, "Content-Type: text/html\r\n", table_string.c_str());
            } else{
                std::vector<monster> sorted_monsters = monsters;

                auto sort_param_it = q_params.find("sort");

                if(sort_param_it != q_params.end()){
                    std::string sort_param = sort_param_it->second;

                    auto dsc_it = sort_param.find("_dsc");

                    if(dsc_it != std::string::npos){
                        auto comp_func_it = query_param_compare_funcs_dsc.find(sort_param.substr(0, dsc_it));

                        if(comp_func_it != query_param_compare_funcs_dsc.end()){
                            std::sort(sorted_monsters.begin(), sorted_monsters.end(), comp_func_it->second);
                        }
                    } else{
                        auto comp_func_it = query_param_compare_funcs_asc.find(sort_param);

                        if(comp_func_it != query_param_compare_funcs_asc.end())
                            std::sort(sorted_monsters.begin(), sorted_monsters.end(), comp_func_it->second);
                    }
                }

                html::element tbody = construct_monster_table_body(sorted_monsters);
                std::string tbody_string = tbody.to_string();

                cache.insert({ cache_key, tbody_string });

                mg_http_reply(c, 200, "Content-Type: text/html\r\n", tbody_string.c_str());
            }
        }

        else if(mg_match(hm->uri, mg_str("/public/#"), caps)){
            std::string file_path = "." + std::string(hm->uri.buf).substr(0, hm->uri.len);

            struct mg_http_serve_opts opts;

            std::fstream requested_file(file_path);

            if(requested_file.good()){
                mg_http_serve_file(c, hm, file_path.c_str(), &opts);
            } else{
                opts = {
                    .mime_types = "html=text/html"
                };

                mg_http_serve_file(c, hm, "./public/pages/notfound.html", &opts);
            }

            requested_file.close();
        }

        else{
            struct mg_http_serve_opts opts = {
                .mime_types = "html=text/html"
            };

            mg_http_serve_file(c, hm, "./public/pages/notfound.html", &opts);
        }
    }
}

void parse_json()
{
    rapidjson::Document monster_json;

    std::ifstream file;
    file.open("./res/monsters.json");

    std::string json;
    std::string line;

    while(getline(file, line)){
        json += line + "\n";
    }

    monster_json.Parse(json.c_str());

    monsters = std::vector<monster>(monster_json.GetArray().Size());

    auto json_trait_to_trait = [](rapidjson::GenericObject<false, rapidjson::Value> json_trait){
        trait trait = {
            name: json_trait["name"].GetString(),
        };

        auto json_text_itr = json_trait.FindMember("text");

        if(json_text_itr != json_trait.MemberEnd()){
            auto& json_text = json_text_itr->value;

            if(json_text.IsArray()){
                auto json_text_arr = json_text.GetArray();

                std::vector<std::string> text(json_text_arr.Size());

                for(unsigned int i = 0; i < json_text_arr.Size(); i++){
                    auto json_trait_str = json_text_arr[i].GetString();
                    text[i] = json_trait_str;
                }

                trait.text = text;
            } else {
                std::vector<std::string> text(1);
                auto json_text_str = json_text.GetString();

                text[0] = json_text_str;

                trait.text = text;
            }
        }

        auto json_attack_itr = json_trait.FindMember("attack");

        if(json_attack_itr != json_trait.MemberEnd()){
            auto& json_attack = json_attack_itr->value;

            if(json_attack.IsArray()){
                auto json_attack_arr = json_attack.GetArray();

                std::vector<std::string> attacks(json_attack_arr.Size());

                for(unsigned int i = 0; i < json_attack_arr.Size(); i++){
                    auto json_attack_str = json_attack_arr[i].GetString();
                    attacks[i] = json_attack_str;
                }

                trait.attacks = attacks;
            } else {
                std::vector<std::string> attacks(1);
                auto json_attack_str = json_attack.GetString();

                attacks[0] = json_attack_str;

                trait.attacks = attacks;
            }
        }

        return trait;
    };

    for(unsigned int i = 0; i < monster_json.GetArray().Size(); i++){
        auto& json_mon = monster_json.GetArray()[i];

        monster mon = {
            name: json_mon["name"].GetString(),
            size: json_mon["size"].GetString(),
            type: json_mon["type"].GetString(),
            alignment: json_mon["alignment"].GetString(),
            ac: json_mon["ac"].GetInt(),
            hp: json_mon["hp"].GetInt(),
            stats: {
                str: json_mon["str"].GetInt(),
                dex: json_mon["dex"].GetInt(),
                con: json_mon["con"].GetInt(),
                _int: json_mon["int"].GetInt(),
                wis: json_mon["wis"].GetInt(),
                cha: json_mon["cha"].GetInt(),
            },
            cr: json_mon["cr"].GetString(),
        };

        auto json_trait_itr = json_mon.FindMember("trait");

        if(json_trait_itr != json_mon.MemberEnd()){
            auto& json_trait = json_trait_itr->value;

            if(json_trait.IsArray()){
                auto json_trait_arr = json_trait.GetArray();

                std::vector<trait> traits(json_trait_arr.Size());

                for(unsigned int i = 0; i < json_trait_arr.Size(); i++){
                    auto json_trait_obj = json_trait_arr[i].GetObject();

                    trait trait = json_trait_to_trait(json_trait_obj);

                    traits[i] = trait;
                }

                mon.traits = traits;
            } else {
                std::vector<trait> traits(1);

                auto json_trait_obj = json_trait.GetObject();
                trait trait = json_trait_to_trait(json_trait_obj);

                traits[0] = trait;

                mon.traits = traits;
            }
        } else{
            mon.traits = std::vector<trait>{};
        }

        monsters[i] = mon;
    }

    file.close();
}

void create_index_html_file()
{
    html::element html("html");

    html::element head("head");
    html::element link("link");
    link.set_attribute("rel", "stylesheet");
    link.set_attribute("href", "/public/styles/main.css");
    head.add_child(link);
    html::element script("script");
    script.set_attribute("src", "/public/scripts/index.js");
    head.add_child(script);

    html::element body("body");
    body.set_attribute("onload", "onLoad()");

    html::element table("table");
    table.set_attribute("id", "monster-table");
    html::element thead("thead");
    html::element tr("tr");

    const char* col_names[7] = {"Name", "CR", "HP", "AC", "Speed", "Swim Speed", "Fly Speed"};

    for(int i = 0; i < 7; i++){
        html::element th("th");
        if(strcmp(col_names[i], "Name") == 0)
            th.set_attribute("style", "width: 20%;");

        th.set_attribute("align", "left");
        th.set_attribute("id", "thead" + std::to_string(i));

        html::element div("div");

        html::element p("p");
        p.add_inner_text(col_names[i]);

        html::element img("img");
        img.set_attribute("src", "/public/images/arrow.png");

        div.add_child(p);
        div.add_child(img);

        th.add_child(div);

        tr.add_child(th);
    }

    thead.add_child(tr);

    html::element tbody = construct_monster_table_body(monsters);

    table.add_child(thead);
    table.add_child(tbody);

    body.add_child(table);

    html.add_child(head);
    html.add_child(body);

    std::ofstream out_file("./public/pages/index.html");
    out_file << html.to_string();
    out_file.close();
}

int main(int argc, char const *argv[])
{
    parse_json();
    create_index_html_file();

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, nullptr);

    while(true){
        mg_mgr_poll(&mgr, 1000);
    }

    return 0;
}
