#include <iostream>
#include <fstream>
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

static void fn(struct mg_connection* c, int ev, void* ev_data)
{
    if(ev == MG_EV_HTTP_MSG){
        struct mg_http_message *hm = (struct mg_http_message*) ev_data;

        std::cout << std::string(hm->query.buf).substr(0, hm->query.len) << std::endl;

        if(mg_strcmp(hm->uri, mg_str("/")) == 0){
            struct mg_http_serve_opts opts = {
                .mime_types = "html=text/html"
            };

            mg_http_serve_file(c, hm, "./public/pages/index.html", &opts);
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

    html::element body("body");

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
        th.add_inner_text(col_names[i]);

        tr.add_child(th);
    }

    thead.add_child(tr);

    html::element tbody("tbody");

    for(monster mon : monsters){
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
