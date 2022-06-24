#include "IncrementalIdMap.hpp"
#include "ContactGraph.hpp"

using gfase::Node;
using gfase::ContactGraph;
using gfase::IncrementalIdMap;
using gfase::random_phase_search;

#include <iostream>

using std::cerr;

void test_mutability(){
    IncrementalIdMap<string> id_map(true);
    ContactGraph g;

    size_t n_nodes = 10;

    for (size_t i=0; i<n_nodes; i++) {
        string a = "n" + to_string(i);

        auto id = id_map.insert(a);
        g.insert_node(int32_t(id));
    }

    for (size_t i=0; i<n_nodes; i++){
        g.try_insert_edge(int32_t(i), int32_t((i+3) % n_nodes));
    }

    g.for_each_node([&](int32_t id, const Node& n){
        cerr << id << '\n';
        cerr << '\t' << "partition: " << int(n.partition) << '\n';
        cerr << '\t' << "neighbors: ";

        g.for_each_node_neighbor(id, [&](int32_t id_other, const Node& n_other){
            cerr << id_other << ' ';
        });

        cerr << '\n';
    });

    cerr << "Edges before editing:" << '\n';
    g.for_each_edge([&](const pair<int32_t,int32_t> e, int32_t weight){
        cerr << e.first << ',' << e.second << ' ' << int(weight) << '\n';
    });

    for (size_t i=5; i<n_nodes; i++){
        g.remove_edge(int32_t(i), int32_t((i+3) % n_nodes));
    }

    cerr << "Edges after editing:" << '\n';
    g.for_each_edge([&](const pair<int32_t,int32_t> e, int32_t weight){
        cerr << e.first << ',' << e.second << ' ' << int(weight) << '\n';
    });

    g.for_each_node([&](int32_t id, const Node& n){
        cerr << id << '\n';
        cerr << n << '\n';
        cerr << '\n';
    });

    for (size_t i=5; i<n_nodes; i++){
        g.try_insert_edge(int32_t(i), int32_t((i+3) % n_nodes));
    }

    cerr << "Edges after un-editing:" << '\n';
    g.for_each_edge([&](const pair<int32_t,int32_t> e, int32_t weight){
        cerr << e.first << ',' << e.second << ' ' << int(weight) << '\n';
    });

    g.for_each_node([&](int32_t id, const Node& n){
        cerr << id << '\n';
        cerr << n << '\n';
        cerr << '\n';
    });

    vector <pair <int32_t, int8_t> > partitions = { {0,-1},
                                                    {1,0},
                                                    {2,1},
                                                    {3,-1},
                                                    {4,0},
                                                    {5,1},
                                                    {6,-1},
                                                    {7,0},
                                                    {8,1},
                                                    {9,-1}};

    g.set_partitions(partitions);

    cerr << "After setting partition:" << '\n';
    g.for_each_node([&](int32_t id, const Node& n){
        cerr << id << '\n';
        cerr << n << '\n';
        cerr << '\n';
    });

    cerr << "Getting partition from graph:" << '\n';
    vector <pair <int32_t, int8_t> > partitions_2;

    g.get_partitions(partitions_2);
    for (auto& [n,p]: partitions_2){
        cerr << n << ',' << int(p) << '\n';
    }

    for (size_t i=5; i<n_nodes; i++){
        g.remove_node(int32_t(i));
    }

    cerr << "After removing nodes" << '\n';
    g.for_each_edge([&](const pair<int32_t,int32_t> e, int32_t weight){
        cerr << e.first << ',' << e.second << ' ' << int(weight) << '\n';
    });

    g.for_each_node([&](int32_t id, const Node& n){
        cerr << id << '\n';
        cerr << n << '\n';
        cerr << '\n';
    });

    g.for_each_edge([&](const pair<int32_t,int32_t> e, int32_t weight){
        g.increment_edge_weight(e.first, e.second, 777);
    });

    cerr << "After incrementing weights:" << '\n';
    g.for_each_edge([&](const pair<int32_t,int32_t> e, int32_t weight){
        cerr << e.first << ',' << e.second << ' ' << int(weight) << '\n';
    });

}


void test_optimization(){
    ContactGraph g;

    /// Haplotype A       (1)    (4)    (7)
    ///                  /   \  /   \  /   \
    /// Unphased      (0)    (3)    (6)    (9)
    ///                 \   /  \   /  \   /
    /// Haplotype B      (2)    (5)    (8)

    vector <pair <int32_t, int8_t> > intended_partitions = {
            {0,0},  // 0
            {1,1},  // 1
            {2,-1}, // 2
            {3,0},  // 3
            {4,1},  // 4
            {5,-1},  // 5
            {6,0},  // 6
            {7,1}, // 7
            {8,-1}, // 8
            {9,0},  // 9
    };

    // Random partitions
    vector<int8_t> partitions = {
            0,  // 0
            0,  // 1
            -1, // 2
            0,  // 3
            1,  // 4
            0,  // 5
            0,  // 6
            -1, // 7
            -1, // 8
            0,  // 9
    };

    for (size_t i=0; i<10; i++){
        g.insert_node(int32_t(i),partitions[i]);
    }

    // Non-phaseable edges
//    g.try_insert_edge(0,1,2);
//    g.try_insert_edge(0,2,2);
//    g.try_insert_edge(3,1,2);
//    g.try_insert_edge(3,2,2);
//
//    g.try_insert_edge(3,4,2);
//    g.try_insert_edge(3,5,2);
//    g.try_insert_edge(6,4,2);
//    g.try_insert_edge(6,5,2);
//
//    g.try_insert_edge(6,7,2);
//    g.try_insert_edge(6,8,2);
//    g.try_insert_edge(9,7,2);
//    g.try_insert_edge(9,8,2);

    // Consistent edges
    g.try_insert_edge(1,4,6);
    g.try_insert_edge(1,7,6);
    g.try_insert_edge(4,7,6);

    // Consistent edges
    g.try_insert_edge(2,5,6);
    g.try_insert_edge(2,8,6);
    g.try_insert_edge(5,8,6);

    // Inconsistent edges
    g.try_insert_edge(1,5,1);
    g.try_insert_edge(4,8,1);
    g.try_insert_edge(2,4,1);
    g.try_insert_edge(5,7,1);

    // Intra-bubble edges
    g.try_insert_edge(1,2,1);
    g.try_insert_edge(4,5,1);
    g.try_insert_edge(7,8,1);

    cerr << "Before optimization:" << '\n';
    g.for_each_node([&](int32_t id, const Node& n){
        cerr << id << '\n';
        cerr << n << '\n';
        cerr << '\n';
    });

    vector <pair <int32_t,int8_t> > best_partitions;
    atomic<int64_t> best_score = std::numeric_limits<int64_t>::min();
    atomic<size_t> job_index = 0;
    mutex phase_mutex;
    size_t m_iterations = 10;

    cerr << g.compute_consistency_score(2) << '\n';

    random_phase_search(g, best_partitions, best_score, job_index, phase_mutex, m_iterations);

    g.set_partitions(best_partitions);
    cerr << "After optimization:" << '\n';
    g.for_each_node([&](int32_t id, const Node& n){
        cerr << id << '\n';
        cerr << n << '\n';
        cerr << '\n';
    });

    for (auto& [n,p]: best_partitions){
        cerr << n << ',' << int(p) << '\n';
    }

    g.set_partitions(intended_partitions);

    cerr << g.compute_consistency_score(2) << '\n';
    cerr << g.compute_total_consistency_score() << '\n';
}


int main(){
    test_mutability();
    test_optimization();

    return 0;
}

