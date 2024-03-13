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

class Utils {

public:

    static bool read_node_oracle(std::string &oracle_filename, const char delimiter, int skip,
                                 emhash5::HashMap<int, int> &node_oracle);

    static bool read_edge_oracle(std::string &oracle_filename, const char delimiter, int skip,
                                 emhash5::HashMap<long, int> &edge_id_oracle);
};


#endif
