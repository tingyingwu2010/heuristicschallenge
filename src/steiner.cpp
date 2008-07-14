/*
 * steiner.cpp
 *
 *  Created on: Jul 12, 2008
 *      Author: pedro
 */

#include <assert.h>
#include <math.h>
#include <fstream>
#include <iostream>

#include <boost/progress.hpp>
#include <boost/multi_array.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>

#include "headers/steiner.hpp"
using namespace std;

void Steiner::generate_chins_solution(Graph& sub_graph, vector<Edge>& spanning_tree) {

	map<int, vector<int> > distances_from_terminal;
	map<int, vector<Vertex> > parents_from_terminal;

	boost::timer timer;
	cout << "pre-calculating distances from all terminals..." << endl;
	for (vector<int>::const_iterator i = terminals.begin(); i != terminals.end(); ++i) {
		int terminal = (*i);
		vector<Vertex> parents(V);
		vector<int> distances(V);

		boost::dijkstra_shortest_paths(graph, terminal, boost::weight_map(boost::get(&EdgeInfo::weight,
				graph)).distance_map(&distances[0]).predecessor_map(&parents[0]));

		distances_from_terminal[terminal] = distances;
		parents_from_terminal[terminal] = parents;
	}
	cout << "distances computed in " << timer.elapsed() << " seconds." << endl;

	vector<int> vertices_in_solution;

	//initialize list temp terminals (TODO: copy in random)
	list<int> terminals_left(terminals.size());
	copy(terminals.begin(), terminals.end(), terminals_left.begin());

	//pick initial terminal
	int t0 = terminals_left.front();
	terminals_left.pop_front();
	vertices_in_solution.push_back(t0);

	while (terminals_left.size() > 0) {
		//choose terminal closest to any of the vertices added
		int closest_distance = INT_MAX;
		int closest_terminal, closest_vertex_in_tree;
		foreach(int t, terminals_left) {

			vector<int> distances = distances_from_terminal[t];
			vector<Vertex> parents = parents_from_terminal[t];

			foreach(int v, vertices_in_solution) {
				if(distances[v] < closest_distance) {
					closest_distance = distances[v];
					closest_terminal = t;
					closest_vertex_in_tree = v;
				}
			}
		}

		assert(closest_distance != INT_MAX);

		terminals_left.remove(closest_terminal);

		//add all vertices in this path (that were not already added)
		Vertex parent = closest_vertex_in_tree, next;
		while(parent != closest_terminal) {
			next = parents_from_terminal[closest_terminal][parent];

			if(find(vertices_in_solution.begin(), vertices_in_solution.end(), parent)==vertices_in_solution.end())
				vertices_in_solution.push_back(parent);

			parent = next;
		}
		//finish adding last vertex in the path
		if(find(vertices_in_solution.begin(), vertices_in_solution.end(), closest_terminal)==vertices_in_solution.end())
			vertices_in_solution.push_back(closest_terminal);
	}


	//create a sub graph structure with only the vertices added so far
	foreach(int v, vertices_in_solution) {
		boost::graph_traits<Graph>::adjacency_iterator vi, viend;
		for (boost::tie(vi,viend) = boost::adjacent_vertices(v, graph); vi != viend; ++vi) {

			if(find(vertices_in_solution.begin(), vertices_in_solution.end(), *vi)!=vertices_in_solution.end()) {

				Edge e; bool found;
				boost::tie(e, found) = boost::edge(v, *vi, graph); assert(found);

				//add edge with same EdgeInfo TODO: find best way to share these info for subgraphs
				boost::add_edge(v, *vi, graph[e], sub_graph);
			}
		}
	}

	//find MST on sub_graph
	boost::kruskal_minimum_spanning_tree(sub_graph, back_inserter(spanning_tree),
			boost::weight_map(boost::get(&EdgeInfo::weight, sub_graph)));


}

int Steiner::find_cost(const Graph& sub_graph, const vector<Edge>& edges) {
	int total = 0;
	foreach(Edge e, edges) {
		total += sub_graph[e].weight;
	}
	return total;
}

Steiner::Steiner(string path) {

	ifstream in_data(path.c_str());

	if (!in_data) {
		cerr << "cannot open file " << path << endl;
		throw "invalid path";
	}

	cout << "reading instance from" << path << endl;

	string line;
	while (getline(in_data, line)) {
		if (line.find("Section Graph") != string::npos) {
			read_graph_section(in_data);
		} else if (line.find("Section Terminals") != string::npos) {
			read_terminals_section(in_data);
		} else if (line.find("Section Coordinates") != string::npos) {
			read_coordinates_section(in_data);
		}
	}

	cout << "done reading." << endl;

	/*
	 cout << "pre-calculating all distances..." << endl;
	 boost::timer t0;

	 vector < int > d(V, (std::numeric_limits < int >::max)());
	 boost::multi_array<int, 2> distances(boost::extents[V][V]);
	 boost::johnson_all_pairs_shortest_paths(graph, distances,
	 boost::distance_map(&d[0]).weight_map(boost::get(&EdgeInfo::weight, graph)) );

	 cout << "all distances computed in " << t0.elapsed() << " seconds." << endl;
	 */

}

inline void Steiner::read_graph_section(ifstream & in_data) {

	in_data.ignore(INT_MAX, ' ') >> V;
	in_data.ignore(INT_MAX, ' ') >> E;

	cout << "instance has " << V << " vertices and " << E << " edges." << endl;

	graph = Graph(V);

	string line;
	int node1, node2, weight;

	for (int i = 0; i < E; i++) {
		in_data.ignore(INT_MAX, ' ') >> node1 >> node2 >> weight;

		//add an edge to the graph
		EdgeInfo edge;
		edge.weight = weight;
		boost::add_edge(node1 - 1, node2 - 1, edge, graph); //indices are 0-based
	}

	assert((int)boost::num_edges(graph) == E);
	assert((int)boost::num_vertices(graph) == V);
}

inline void Steiner::read_terminals_section(ifstream & in_data) {

	int number_terminals;
	in_data.ignore(INT_MAX, ' ') >> number_terminals;

	cout << "instance has " << number_terminals << " terminals." << endl;

	int terminal_node;
	for (int i = 0; i < number_terminals; i++) {
		in_data.ignore(INT_MAX, ' ') >> terminal_node; //indices are 0-based
		terminals.push_back(terminal_node - 1);
	}

	assert((int)terminals.size() == number_terminals);

}

inline void Steiner::read_coordinates_section(ifstream & in_data) {

	//only for drawing
	for (int i = 0; i < V; i++) { /* indices are 0-based */
		in_data.ignore(INT_MAX, ' ').ignore(INT_MAX, ' ') >> graph[i].x >> graph[i].y;
	}

}

Steiner::~Steiner() {
}
