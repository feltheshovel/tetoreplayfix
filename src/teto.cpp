#include <iostream>
#include "json.hpp"
#include <fstream>
#include <string>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    std::string file;
    if(argc == 1) {
        std::cout << "file name: ";
        std::cin >> file;
    } else {
        file = argv[1];
    }
    std::ifstream input(file);
    if(!input.is_open()) {
        std::cerr << "error: failed to open file \"" << file << "\"(line 18)" << std::endl;
        return 1;
    }
    json before = json::parse(input);
    input.close();

    json after = json::object();

    //after
    after["users"] = json::array();
    after["replay"] = json::object();
    after["version"] = 1;
    after["ts"] = before["ts"];
    
    //replay
    after["replay"]["leaderboard"] = json::array();
    after["replay"]["rounds"] = json::array();

    //lbstats
    std::string name[2];  //name[id]
    int wins[2]={0};  //wins[id]
    std::string ida[2];  //ida[i]
    double apm[2]={0}, pps[2]={0}, vs[2]={0};  //stat[id]
    for(int i = 0 ; i < 2 ; i++) {
        ida[i] = before["data"][0]["board"][i]["user"]["_id"];
        name[i] = before["data"][0]["board"][i]["user"]["username"];
    }

    //rounds
    /*rounds:[
        k:[
            0:{}
            1:{}
        ]
    ]
    */
    for(const auto& i : before["data"]) {
        after["replay"]["rounds"].push_back(json::array());
        for(int j = 0 ; j < 2 ; j++) {  //player
            bool id = ida[1]==i["board"][j]["user"]["_id"];
            double lapm = i["replays"][j]["events"].back()["data"]["export"]["aggregatestats"]["apm"];
            apm[id] += lapm;
            double lpps = i["replays"][j]["events"].back()["data"]["export"]["aggregatestats"]["pps"];
            pps[id] += lpps;
            double lvs = i["replays"][j]["events"].back()["data"]["export"]["aggregatestats"]["vsscore"];
            vs[id] += lvs;
            bool win = i["board"][j]["success"];
            wins[id] += win;
            int frames = i["replays"][j]["frames"];

            //rounds[k][j]
            after["replay"]["rounds"].back().push_back({
                {"id",ida[id]},
                {"username",i["board"][j]["user"]["username"]},
                {"active",true},
                {"alive",win},
                {"lifetime",frames*1000/60+1},
                {"stats",{
                    {"apm",lapm},
                    {"pps",lpps},
                    {"vsscore",lvs}
                }},
                {"replay",{
                    {"frames",frames},
                    {"events",json::array({
                        {{"frame", 0},
                        {"type", "start"},
                        {"data", {}}},

                        {{"frame", 0},
                        {"type", "ige"},
                        {"data", {
                            {"id", 0},
                            {"frame", 0},
                            {"type", "target"},
                            {"data", {{"targets", {5956}}}}
                        }}},

                        {{"frame", 0},
                        {"type", "ige"},
                        {"data", {
                            {"id", 1},
                            {"frame", 0},
                            {"type", "allow_targeting"},
                            {"data", {{"value", false}}}
                        }}},

                        {{"frame", 0},
                        {"type", "ige"},
                        {"data", {
                            {"id", 2},
                            {"frame", 0},
                            {"type", "target"},
                            {"data", {{"targets", {5956}}}}
                        }}},

                        {{"frame", 0},
                        {"type", "ige"},
                        {"data", {
                            {"id", 3},
                            {"frame", 0},
                            {"type", "allow_targeting"},
                            {"data", {{"value", false}}}
                        }}},
                    })},
                    {"options",json::object({})}
                }}
            });

            //options
            std::string useless[] = {"ghostskin","noscope","objective","physical","slot_counter4"};
            std::string sc[] = {"display_attack","display_kills",
            "display_lines","display_pieces","display_placement","display_stopwatch",
            "display_vs"};
            int scounter = 1;
            for(const auto& k : i["replays"][j]["events"][0]["data"]["options"].items()) {
                bool success = 1;
                for(const auto& l : useless) {
                    if(k.key() == l) {
                        success = 0;
                        break;
                    }
                }
                if(!success) {
                    continue;
                }
                success = 0;
                for(const auto& l : sc) {
                    if(k.key() == l) {
                        if(k.value()) {
                            after["replay"]["rounds"].back().back()["replay"]["options"].emplace
                            ("slot_counter"+std::to_string(scounter),k.key().substr(8,k.key().length()));
                            scounter++;
                        }
                        success = 1;
                        break;
                    }
                }
                if(success) {
                    continue;
                }
                if(k.key()=="display_impending") {
                    after["replay"]["rounds"].back().back()["replay"]["options"].emplace("slot_bar1","impending");
                    continue;
                }
                if(k.key()=="latencypreference") {
                    after["replay"]["rounds"].back().back()["replay"]["options"].emplace("latencymode",k.value());
                    continue;
                }
                if(k.key()=="infinitemovement") {
                    after["replay"]["rounds"].back().back()["replay"]["options"].emplace("infinite_movement",k.value());
                    continue;
                }
                if(k.key()=="countdonw_interval") {
                    after["replay"]["rounds"].back().back()["replay"]["options"].emplace("countdown_interval",k.value());
                    continue;
                }
                after["replay"]["rounds"].back().back()["replay"]["options"].emplace(k.key(),k.value());
            }
            after["replay"]["rounds"].back().back()["replay"]["options"].emplace
            ("handling",i["replays"][j]["events"][0]["data"]["game"]["handling"]);

            //events
            int eid = 1;
            for(const auto& k : i["replays"][j]["events"]) {
                if(eid < 4) {  //skip the first 3
                    eid++;
                    continue;
                }
                if(k["type"] == "ige") {
                    after["replay"]["rounds"].back().back()["replay"]["events"].push_back(k["data"]);
                    after["replay"]["rounds"].back().back()["replay"]["events"].back().erase("id");
                    after["replay"]["rounds"].back().back()["replay"]["events"].back().erase("key");
                    after["replay"]["rounds"].back().back()["replay"]["events"].back()["data"].erase("cid");
                    after["replay"]["rounds"].back().back()["replay"]["events"].back()["data"].emplace("id",eid);
                    eid++;
                    continue;
                }
                after["replay"]["rounds"].back().back()["replay"]["events"].push_back(k);
            }

            //results
            after["replay"]["rounds"].back().back()["replay"]["events"].back()["data"].erase("export");
            after["replay"]["rounds"].back().back()["replay"]["events"].back()["data"].erase("reason");
            after["replay"]["rounds"].back().back()["replay"].emplace
            ("results",i["replays"][j]["events"].back()["data"]["export"]);
            after["replay"]["rounds"].back().back()["replay"]["results"].erase("reason");
            after["replay"]["rounds"].back().back()["replay"]["results"].emplace
            ("gameoverreason",i["replays"][j]["events"].back()["data"]["reason"]);
        }
    }

    //leaderboard
    for(int i = 0 ; i < 2 ; i++) {
        after["replay"]["leaderboard"].push_back(json::object({
            {"id",ida[i]},
            {"username",name[i]},
            {"active",true},
            {"wins",wins[i]},
            {"stats",{
                {"apm",apm[i]/before["data"].size()},
                {"pps",pps[i]/before["data"].size()},
                {"vsscore",vs[i]/before["data"].size()},
            }}
        }));
    }

    //users
    for(int i = 0 ; i < 2 ; i++) {
        after["users"].push_back(json::object({
            {"id",ida[i]},
            {"username",name[i]}
        }));
    }

    std::ofstream out("out.ttrm",std::ios::trunc);
    if(!out.is_open()) {
        std::cerr << "error: failed to open file \"out.ttrm\" (line 235)" << std::endl;
        return 1;
    }
    out << after;
    out.close();
    
    std::cout << "done" << std::endl;

    return 0;
}