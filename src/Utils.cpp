//
// Created by Cristian Boldrin on 09/03/24.
//

#include "../include/Utils.h"

bool Utils::read_node_oracle(std::string &oracle_filename, const char delimiter, int skip,
                             emhash5::HashMap<int, int> &node_oracle) {

    std::ifstream file(oracle_filename);
    std::string line;
    int i = 0;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (i >= skip) {
                std::istringstream iss(line);
                std::string token;
                std::getline(iss, token, delimiter);
                int node = std::stoi(token);
                std::getline(iss, token, delimiter);
                int label = std::stoi(token);
                node_oracle.insert_unique(node, label);
            }
            i++;
        }
        file.close();
        return true;
    } else {
        std::cerr << "Error! Unable to open file " << oracle_filename << "\n";
        return false;
    }

}

bool Utils::read_edge_oracle(std::string &oracle_filename, const char delimiter, int skip,
                             emhash5::HashMap<long, int> &edge_id_oracle) {

    std::ifstream file(oracle_filename);
    std::string line;
    int i = 0;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (i >= skip) {
                std::istringstream iss(line);
                std::string token;
                std::getline(iss, token, delimiter);
                int u = std::stoi(token);
                std::getline(iss, token, delimiter);
                int v = std::stoi(token);
                std::getline(iss, token, delimiter);
                int label = std::stoi(token);
                edge_id_oracle.insert_unique(TriangleSampler::edge_to_id(u, v), label);
            }
            i++;
        }
        file.close();
        return true;
    } else {
        std::cerr << "Error! Unable to open file " << oracle_filename << "\n";
        return false;
    }

}