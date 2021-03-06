/*
 * steiner_grasp.cpp
 *
 *  Created on: Jul 28, 2008
 *      Author: pedro
 */

#include <assert.h>
#include <math.h>
#include <iostream>

#include <boost/graph/random.hpp>
#include <boost/random.hpp>
#include <boost/progress.hpp>

#include <boost/format.hpp>

#include "headers/steiner.hpp"
#include "headers/steiner_solution.hpp"
#include "headers/steiner_heuristics.hpp"
#include "headers/steiner_nodesearch.hpp"
#include "headers/steiner_pathsearch.hpp"
#include "headers/steiner_grasp.hpp"

using namespace std;

SteinerGRASP::SteinerGRASP(Steiner& steiner) :
	instance(steiner) {
}

void SteinerGRASP::run() {

	size_t max_iterations = 20;
	int k = 0;

	best_cost = INT_MAX;
	size_t best_root;
	boost::timer t0;

	SteinerNodeLocalSearch node_ls;
	SteinerSolution solution = SteinerHeuristics::generate_network_distance_solution(instance);

	cout << "cost initial solution: " << solution.find_cost() << endl;

	for (size_t i = 0; i < max_iterations; i++) {

		SteinerSolution new_solution = node_ls.key_node_insert(solution);

		int cost = new_solution.find_cost();

		//SteinerPathLocalSearch::search(new_solution);
		//cost = new_solution.find_cost();

		if (cost < best_cost) {
			cout << "*** new current best " << cost;
			cout << "   V: " << new_solution.graph.num_vertices();
			cout << " E: " << new_solution.graph.num_edges() << "\n";

			best_cost = cost;
			solution = new_solution;

			if(cost == instance.best_known)
				break;
		}

		k++;
	}

	cout << "GRASP finished in " << k << " total iterations in " << t0.elapsed() << " seconds. \n";
	cout << "best found was " << best_cost << "\n";

	solution.check_integrity();
	solution.graph.writedot((boost::format("best_%1%.dot") % best_cost).str());
	solution.writetex((boost::format("best_%1%.tex") % best_cost).str());

	cout << endl;
}
