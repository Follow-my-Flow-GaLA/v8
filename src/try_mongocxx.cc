#include <cstdint>
#include <iostream>
#include <vector>
#include <regex>
#include <string.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;


mongocxx::instance instance{}; // This should be done only once.
mongocxx::uri uri("mongodb://127.0.0.1:27017");
mongocxx::client client(uri);
auto db = client["phase1"];
auto undef_prop_dataset_collection = db["undef_prop_dataset"];
auto phase_info_collection = db["phase_info"];


void add_log_to_undef_prop_dataset(std::map<std::string, std::string> data) {
    auto row_col_str = data["row"] + "," + data["col"];

    auto code_hash_obj = undef_prop_dataset_collection.find_one(make_document(kvp("_id", data["code_hash"])));
    if (code_hash_obj) {
        auto code_hash_value = code_hash_obj->view();
        auto key_value_dict_view = code_hash_value["key_value_dict"].get_document().view();
        bsoncxx::builder::basic::array new_row_col_list{};
        // code_hash already exists, check if key exists
        if (key_value_dict_view.find(data["key"]) != key_value_dict_view.end()) {
            // key also exists, check if row_col exists
            auto row_col_list = key_value_dict_view[data["key"]].get_array().value;
            bool row_col_exists = false;
            for (auto& element : row_col_list) {
                if (std::string(element.get_string().value.to_string()) == row_col_str) {
                    row_col_exists = true;
                    break;
                }
                new_row_col_list.append(element.get_value());
            }

            if (!row_col_exists) {
                new_row_col_list.append(row_col_str); // Add new row_col_str
            }
        } else {
            // key does not exist, add it
            new_row_col_list.append(row_col_str);
        }
        // Update the document
        bsoncxx::builder::basic::document new_key_value_dict{};
        for (auto& element : key_value_dict_view) {
            new_key_value_dict.append(kvp(element.key(), element.get_value()));
        }
        new_key_value_dict.append(kvp(data["key"], new_row_col_list.extract()));

        undef_prop_dataset_collection.update_one(
            make_document(kvp("_id", data["code_hash"])),
            make_document(kvp("$set", make_document(kvp("key_value_dict", new_key_value_dict.extract()))))
        );

    } else {
        // code_hash does not exist, add it
        undef_prop_dataset_collection.insert_one(
        make_document(
            kvp("_id", data["code_hash"]), 
            kvp("key_value_dict", make_document(kvp(data["key"], make_array(row_col_str))))
        )
    );
    }
}

void add_log_to_phase_info(std::map<std::string, std::string> data) {
    auto row_col_str = data["row"] + "," + data["col"];

    auto site_obj = phase_info_collection.find_one(make_document(kvp("_id", data["site"])));
    if (site_obj) {
        auto site_value = site_obj->view();
        auto code_hash_dict_view = site_value["code_hash_dict"].get_document().view();
        // site already exists, check if code_hash exists
        if (code_hash_dict_view.find(data["code_hash"]) != code_hash_dict_view.end()) {
            // code_hash also exists, check if key exists
            auto key_value_dict_view = code_hash_dict_view[data["code_hash"]].get_document().view();
            bsoncxx::builder::basic::array new_row_col_list{};
            if (key_value_dict_view.find(data["key"]) != key_value_dict_view.end()) {
                // key also exists, check if row_col exists
                auto row_col_list = key_value_dict_view[data["key"]].get_array().value[0].get_array().value;
                bool row_col_exists = false;
                for (auto& element : row_col_list) {
                    if (std::string(element.get_string().value.to_string()) == row_col_str) {
                        row_col_exists = true;
                        break;
                    }
                    new_row_col_list.append(element.get_value());
                }
                // if row_col does not exist, add it
                if (!row_col_exists) {
                    new_row_col_list.append(row_col_str);
                    phase_info_collection.update_one(
                        make_document(kvp("_id", data["site"])),
                        make_document(kvp("$set", make_document(kvp("code_hash_dict." + data["code_hash"] + "." + data["key"] + ".0", new_row_col_list.extract())))
                    ));
                }
            } else {
                // key does not exist, add it
                new_row_col_list.append(row_col_str);
                phase_info_collection.update_one(
                    make_document(kvp("_id", data["site"])),
                    make_document(kvp("$set", make_document(kvp("code_hash_dict." + data["code_hash"] + "." + data["key"], make_array(new_row_col_list.extract(), data["js"], data["func_name"], data["func"])))))
                );
            }
        } else {
            // code_hash does not exist, add it
            phase_info_collection.update_one(
                make_document(kvp("_id", data["site"])),
                make_document(kvp("$set", make_document(kvp("code_hash_dict." + data["code_hash"], make_document(kvp(data["key"], make_array(make_array(row_col_str), data["js"], data["func_name"], data["func"]))))))
            ));
        }
    } else {
        // site does not exist, add it
        phase_info_collection.insert_one(
            make_document(
                kvp("_id", data["site"]), 
                kvp("code_hash_dict", make_document(kvp(data["code_hash"], make_document(kvp(data["key"], 
                    make_array(make_array(row_col_str), data["js"], data["func_name"], data["func"]))))))));
    }
}


// this is the same as the phase1/undefined route in the backend server
std::string log_phase1_db(std::map<std::string, std::string> data) {
    auto required_fields = {"phase", "start_key", "site", "key", "func_name", "js", "row", "col", "func", "code_hash"};
    for (auto field : required_fields) {
        if (data.find(field) == data.end()) {
            return std::string("Missing required field: ") + std::string(field);
        }
    }

    // check if the phase is valid (phase == "1")
    // TODO: change to phase % 2 == 1 (convert to int first)
    if (data["phase"].compare("1") != 0) {
        return "Invalid phase";
    }

    // check if the start_key is valid
    auto valid_start_keys = {"RTO", "RAP0", "RAP1", "JRGDP", "OGPWII", "GPWFAC"};
    if (std::find(valid_start_keys.begin(), valid_start_keys.end(), data["start_key"]) == valid_start_keys.end()) {
        return "Invalid start_key";
    }

    // sanitize site, change dots to underscores
    data["site"] = std::regex_replace(data["site"], std::regex("\\."), "_");

    // sanitize all other fields, change dots to \\2E, change dollar signs to \\24
    for (auto field : data) {
        if (field.first.compare("site") != 0) {
            data[field.first] = std::regex_replace(data[field.first], std::regex("\\."), "\\2E");
            data[field.first] = std::regex_replace(data[field.first], std::regex("\\$"), "\\24");
        }
    }

    // add log to undef_prop_dataset
    add_log_to_undef_prop_dataset(data);

    // add log to phase_info 
    add_log_to_phase_info(data);

    return "Success";        
}

int main() {
    auto test_data = std::map<std::string, std::string>{
        {"phase", "1"},
        {"start_key", "RTO"},
        {"site", "site1"},
        {"key", "key2"},
        {"func_name", "func1"},
        {"js", "js1"},
        {"row", "row1"},
        {"col", "col1"},
        {"func", "func1"},
        {"code_hash", "code_hash1"}
    };

    std::cout << log_phase1_db(test_data) << std::endl;

    return 0;
}
