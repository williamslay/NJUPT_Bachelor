/**
 * This is a C++ port of distance.py from
 * https://github.com/aflgo/aflgo/blob/master/scripts/distance.py
 *
 * Loris Reiff <loris.reiff@liblor.ch>
 */

#include <boost/program_options.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>

#include <iostream>
#include <string>
#include <unordered_map>
#include <queue>

namespace po = boost::program_options;
namespace bo = boost;
using std::cout;
using std::cerr;
using std::exception;
using std::unordered_map;


struct Vertex {
    std::string name, label, shape;
};

struct Edge {
    std::string label;
    double weight;
};
typedef bo::property<bo::graph_name_t, std::string> graph_p;
typedef bo::adjacency_list<bo::vecS, bo::vecS,bo::directedS, Vertex, Edge,graph_p> graph_t;
typedef bo::graph_traits<graph_t>::vertex_descriptor vertex_desc;
typedef bo::graph_traits<graph_t>::edge_descriptor edge_desc;

static bool is_cg;

static inline std::string node_name(const std::string &name) {
    if (is_cg) {
        return "{" + name + "}";
    } else {
        return "{" + name + ":";
    }
}

std::vector<vertex_desc> find_nodes(const graph_t &G, const std::string &name){
    std::string n_name = node_name(name);
    // memoization
    static unordered_map<std::string, std::vector<vertex_desc>> mem;
    auto ver_itr = mem.find(n_name);
    if (ver_itr != mem.end()) return ver_itr->second;

    std::vector<vertex_desc> ret;
    bo::graph_traits<graph_t>::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(G); vi != vi_end; ++vi) {
        if(G[*vi].label.find(n_name) != std::string::npos) {
            ret.push_back(*vi);
        }
    }
    mem[n_name] = ret;
    return ret;
}

// for callsit_test
void cg_disinit(graph_t &G, std::ifstream &stream)
{
    typedef bo::graph_traits<graph_t>::edge_iterator EdgeIterator;
     std::pair<EdgeIterator, EdgeIterator> ei;
      for (ei = edges(G); ei.first != ei.second; ++ei.first) {
        G[*ei.first].weight=0.0;
    }
    for (std::string line; getline(stream, line); ) {
        bo::trim(line);
        std::vector<std::string> splits;
        bo::algorithm::split(splits, line, bo::is_any_of(","));
        assert(splits.size() == 4); 
        std::string factor1 = splits[2]; 
        std::string factor2 = splits[3];
        if (find_nodes(G, splits[0]).empty()||find_nodes(G, splits[1]).empty()) continue;
        vertex_desc v1 = find_nodes(G, splits[0]).front();
        vertex_desc v2 = find_nodes(G, splits[1]).front(); 
        double cn = static_cast<double>(factor1[0] - '0');
        double cb = static_cast<double>(factor2[0] - '0');
        auto e = edge(v1,v2,G).first;
        bool t = edge(v1,v2,G).second; 
        if(t)
        {
            double weight = ((2*cn+1)/(2*cn))*((2*cb+1)/(2*cb)); 
            G[e].weight = weight; 
        }
    } 
} 
//for trace closure
std::vector<vertex_desc> tg_closure(
    graph_t &G,
    std::vector<vertex_desc> &targets
) {
    std::vector<vertex_desc> reachable; 
    std::queue<vertex_desc> worklist;
    for (vertex_desc target:targets)
    {

        worklist.push(target);
        reachable.push_back(target);  
        while(!worklist.empty())
        { 
            vertex_desc temp = worklist.front();
            worklist.pop(); 
            bo::graph_traits<graph_t>::edge_iterator ei, ei_end;
            for (bo::tie(ei, ei_end) = boost::edges(G); ei != ei_end; ++ei) {
               vertex_desc s = boost::source(*ei, G);
               vertex_desc t = boost::target(*ei, G);
               if(t == temp)
               {
                 auto result = std::find(reachable.begin(), reachable.end(), s);
                 if (result != reachable.end())
                 {
                    continue;
                 }
                 else
                 {
                    worklist.push(s);
                    reachable.push_back(s); 
                }
               }
            }
        }
    }
    std::sort(reachable.begin(), reachable.end());
    // Remove duplicates using std::unique()
    auto it = std::unique(reachable.begin(), reachable.end());
    // Erase the removed elements using std::erase()
    reachable.erase(it, reachable.end());
    return  reachable; 
}
// inline void init_distances_from(const graph_t &G, vertex_desc from, std::vector<int> &dists) {
//     auto dist_pmap = bo::make_iterator_property_map(dists.begin(), get(bo::vertex_index, G));
//     auto vis = bo::make_bfs_visitor(bo::record_distances(dist_pmap, bo::on_tree_edge()));
//     bo::breadth_first_search(G, from, bo::visitor(vis));
// }

inline void init_distances_from(const graph_t &G, vertex_desc from, std::vector<double> &dists) { 
    std::vector<vertex_desc> parent(num_vertices(G));
    bo::dijkstra_shortest_paths(G, from,bo::predecessor_map(bo::make_iterator_property_map(parent.begin(), bo::get(bo::vertex_index, G))).
                                        distance_map(bo::make_iterator_property_map(dists.begin(), bo::get(bo::vertex_index, G))).
                                        weight_map(get(&Edge::weight, G)));
}

void distance(
    const graph_t &G,
    const std::string &name,
    const std::vector<vertex_desc> &targets,
    std::ofstream &out,
    unordered_map<std::string, double> &bb_distance
) {
    if (not is_cg and bb_distance.find(name) != bb_distance.end()) {
        out << name << "," << bo::lexical_cast<std::string>(10 * bb_distance[name]) << "\n";
        return;
    }

    double distance = -1;
    for (vertex_desc n : find_nodes(G, name)) {
        std::vector<double> distances(bo::num_vertices(G), 0.0);
        init_distances_from(G, n, distances);
        double d = 0.0;
        unsigned i = 0;
        if (is_cg) {
            for (vertex_desc t : targets) {
                auto shortest = distances[t];           // shortest distance from n to t
                if (shortest == 0 and n != t) continue; // not reachable
                if (shortest > 1e+10 ) continue; 
                d += 1.0 / (1.0 + static_cast<double>(shortest));
                ++i;
            }
        } else {
            for (auto &bb_d_entry : bb_distance) {
                double di = 0.0;
                unsigned ii = 0;
                for (auto t : find_nodes(G, bb_d_entry.first)) {
                    auto shortest = distances[t];           // shortest distance from n to t
                    if (shortest == 0 and n != t) continue; // not reachable
                    if (shortest > 1e+10 ) continue; 
                    di += 1.0 / (1.0 + 10 * bb_d_entry.second + static_cast<double>(shortest));
                    ++ii;
                }
                if (ii != 0) {
                    d += di / static_cast<double>(ii);
                    ++i;
                }
            }
        }
        double tmp = static_cast<double>(i) / d;
        if (d != 0 and (distance == -1 or distance > tmp)) {
            distance = tmp;
        }
    }

    if (distance != -1) {
        out << name << "," << bo::lexical_cast<std::string>(distance) << "\n";
    }
}

std::vector<vertex_desc> cg_calculation(
    graph_t &G,
    std::ifstream &target_stream
) {
    cout << "Loading targets..\n";
    std::vector<vertex_desc> targets;
    for (std::string line; getline(target_stream, line); ) {
        bo::trim(line);
        for (auto t : find_nodes(G, line)) {
            targets.push_back(t);
        }
    }
    if (targets.empty()) {
        cout << "No targets available\n";
        exit(0);
    }
    return targets;
}

std::vector<vertex_desc> cfg_calculation(
    graph_t &G,
    std::ifstream &targets_stream,
    std::ifstream &cg_distance_stream,
    std::ifstream &cg_callsites_stream,
    unordered_map<std::string, double> &cg_distance,
    unordered_map<std::string, double> &bb_distance
) {
    std::vector<vertex_desc> targets;
    for (std::string line; getline(cg_distance_stream, line); ) {
        bo::trim(line);
        std::vector<std::string> splits;
        bo::algorithm::split(splits, line, bo::is_any_of(","));;
        assert(splits.size() == 2);
        cg_distance[splits[0]] = std::stod(splits[1]);
    }
    if (cg_distance.empty()) {
        cerr << "Call graph distance file is empty.\n";
        exit(0);
    }

    for (std::string line; getline(cg_callsites_stream, line); ) {
        bo::trim(line);
        std::vector<std::string> splits;
        bo::algorithm::split(splits, line, bo::is_any_of(","));;
        assert(splits.size() == 2);
        if (not find_nodes(G, splits[0]).empty()) {
            if (cg_distance.find(splits[1]) != cg_distance.end()) {
                if (bb_distance.find(splits[0]) != bb_distance.end()) {
                    if (bb_distance[splits[0]] > cg_distance[splits[1]]) {
                        bb_distance[splits[0]] = cg_distance[splits[1]];
                    }
                } else {
                    bb_distance[splits[0]] = cg_distance[splits[1]];
                }
            }
        }
    }
    cout << "Adding target BBs (if any)..\n";
    for (std::string line; getline(targets_stream, line); ) {
        bo::trim(line);
        std::vector<std::string> splits;
        bo::algorithm::split(splits, line, bo::is_any_of("/"));;
        size_t found = line.find_last_of('/');
        if (found != std::string::npos)
            line = line.substr(found+1);
        if (not find_nodes(G, splits[0]).empty()) {
            bb_distance[line] = 0.0;
            cout << "Added target BB " << line << "!\n";
        }
    }
    return targets;
}

std::ifstream open_file(const std::string &filename) {
    std::ifstream filestream(filename);
    if (not filestream) {
        cerr << "Error: " << strerror(errno) << ": " << filename << "\n";
        exit(1);
    }
    return filestream;
}

int main(int argc, char *argv[]) {
    po::variables_map vm;
    try {
        po::options_description desc("AFLGo distance calculator Port");
        desc.add_options()
                ("help,h", "produce help message")
                ("dot,d", po::value<std::string>()->required(), "Path to dot-file representing the "
                                                           "graph.")
                ("targets,t", po::value<std::string>()->required(), "Path to file specifying Target"
                                                                    " nodes.")
                ("out,o", po::value<std::string>()->required(), "Path to output file containing "
                                                                "distance for each node.")                        
                ("names,n", po::value<std::string>()->required(), "Path to file containing name for"
                                                                  " each node.")
                ("fcallconuts,f", po::value<std::string>()->required(), "Path to file containing function callsits"
                                                                  " counts.")
                ("closure,O", po::value<std::string>(), "Path to output file containing "
                                                                "trace closure.")        
                ("cg_distance,c", po::value<std::string>(), "Path to file containing call graph "
                                                            "distance.")
                ("cg_callsites,s", po::value<std::string>(), "Path to file containing mapping "
                                                             "between basic blocks and called "
                                                             "functions.")
                ;

        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }
        po::notify(vm);
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }

    std::ifstream dot = open_file(vm["dot"].as<std::string>());
    cout << "Parsing " << vm["dot"].as<std::string>() << " ..\n";
    graph_t graph(0);
    bo::dynamic_properties dp(bo::ignore_other_properties);
    dp.property("node_id", get(&Vertex::name,  graph));
    dp.property("label",   get(&Vertex::label, graph));
    dp.property("shape",   get(&Vertex::shape, graph));
    dp.property("label",   get(&Edge::label,   graph)); 
    boost::ref_property_map<graph_t *, std::string> gname(get_property(graph, bo::graph_name));
    dp.property("label",    gname);   

    if (!read_graphviz(dot, graph, dp)) {
        cerr << "Error while parsing " << vm["dot"].as<std::string>() << std::endl;
        return 1;
    }
    is_cg = get_property(graph, bo::graph_name).find("Call graph") != std::string::npos;
    cout << "Working on " << (is_cg ? "callgraph" : "control flow graph") << "\n";

    std::ifstream targets_stream = open_file(vm["targets"].as<std::string>());
    std::ifstream names = open_file(vm["names"].as<std::string>());
    std::ifstream callcouts_stream = open_file(vm["fcallconuts"].as<std::string>());
    std::vector<vertex_desc> targets;
    unordered_map<std::string, double> cg_distance;
    unordered_map<std::string, double> bb_distance;

    if (is_cg) {
        if (not vm.count("closure")) {
            cerr << "error: the required argument for option '--closure' is missing\n";
            exit(1);
        }
        std::ofstream closure_stream(vm["closure"].as<std::string>());
        std::vector<vertex_desc> traces; 
        targets = cg_calculation(graph, targets_stream);
        traces = tg_closure(graph,targets);
        for(vertex_desc trace : traces)
        {
            closure_stream << graph[trace].label<<"\n";  
        }
        cg_disinit(graph,callcouts_stream);
    } else {
        if (not vm.count("cg_callsites")) {
            cerr << "error: the required argument for option '--cg_callsites' is missing\n";
            exit(1);
        }
        if (not vm.count("cg_distance")) {
            cerr << "error: the required argument for option '--cg_distance' is missing\n";
            exit(1);
        }
        std::ifstream cg_distance_stream = open_file(vm["cg_distance"].as<std::string>());
        std::ifstream cg_callsites_stream = open_file(vm["cg_callsites"].as<std::string>());

        std::vector<std::string> splits;
        bo::algorithm::split(splits, vm["dot"].as<std::string>(), bo::is_any_of("."));;
        std::string &caller = splits.end()[-2];
        cout << "Loading cg_distance for function '" << caller << "'..\n";
        targets = cfg_calculation(graph, targets_stream, cg_distance_stream,
                                  cg_callsites_stream, cg_distance, bb_distance);
    }

    cout << "Calculating distance..\n";
    std::ofstream outstream(vm["out"].as<std::string>());
    for (std::string line; getline(names, line); ) {
        bo::trim(line);
        distance(graph, line, targets, outstream, bb_distance);
    }

    return 0;
}
