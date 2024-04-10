//
// Created by Cristian Boldrin on 09/03/24.
//

#ifndef TONIC_UTILS_H
#define TONIC_UTILS_H

#include "hash_table5.hpp"
#include "TriangleSampler.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_set>

struct hash_edge {
    size_t operator()(const std::pair<int, int> &p) const {
        return TriangleSampler::edge_to_id(p.first, p.second);
    }
};

class Utils {

public:

    static long run_exact_algorithm(std::string &dataset_filepath,  std::string &output_path);

    static bool read_node_oracle(std::string &oracle_filename, char delimiter, int skip,
                                 emhash5::HashMap<int, int> &node_oracle);

    static bool read_edge_oracle(std::string &oracle_filename, char delimiter, int skip,
                                 emhash5::HashMap<long, int> &edge_id_oracle);

    static void preprocess_data(const std::string &dataset_path, std::string &delimiter,
                                int skip, std::string &output_path);

    static void build_edge_exact_oracle(std::string &filepath, double percentage_retain,
                                  std::string &output_path);

    static void build_node_oracle(std::string &filepath, double percentage_retain,
                                        std::string &output_path);
};


#endif
