/*!
\author Rémi Attab
\date 6/03/2011
\license FreeBSD (see LICENSE file).

This file contains the solution to the third dropbox challenge.

How it all works:

We first split the input set into two categories, positive and negative values.
Using dynamic programming, we then compute in parallel every permutation of 
the positive and negative values. If we find that a permutation of positive values
sum up to the sum of a permutation of negative values, then we have our solution.

Our dynamic programming solution uses a memoization table where it's key is the 
sumed value of the activities kept in the node. Since we only deal with absolute
value, the node also has an attribute that indicate whether we're dealing with a
positive node or negative node.
 */


#include <iostream>
#include <sstream>

#include <set>
#include <map>
#include <string>
#include <algorithm>

#include <cstdlib>
#include <cstdio>


// Typedef hell

typedef int t_activity_id;
typedef int t_cvalue; // short for caloric_value.

typedef std::set<t_activity_id> t_activity_set;
typedef t_activity_set::iterator t_activity_it;
typedef t_activity_set::const_iterator t_activity_cit;

typedef std::map<t_activity_id, t_cvalue> t_cvalue_map;
typedef t_cvalue_map::iterator t_cvalue_it;
typedef t_cvalue_map::const_iterator t_cvalue_cit;

typedef std::map<t_activity_id, std::string> t_name_map;


struct t_node;
typedef std::map<t_cvalue, t_node> t_cvalue_table;
typedef t_cvalue_table::value_type t_cvalue_table_pair;




//! Represents either a positive or negative entry in the DP memoization table.
struct t_node {
  t_node() : is_positive(false), activities() {}
  t_node(bool _is_positive) :
    is_positive(_is_positive), 
    activities()
  {}
  t_node (const t_node& src) :
    is_positive(src.is_positive),
    activities(src.activities.begin(), src.activities.end())
  {}
  
  bool is_positive;
  t_activity_set activities;

  t_node& operator= (const t_node& src) {
    if (this == &src) return *this;
    is_positive = src.is_positive;
    activities = t_activity_set(src.activities.begin(), src.activities.end());
    return *this;
  }
};


//If using boost would be a boost::noncopyable.
struct t_algo_state {
  t_cvalue_map plus_map;
  t_cvalue_map minus_map;
  t_name_map name_map;
};


// Prototypes

void run_tests ();
void add_to_state (t_algo_state& state, 
		   t_activity_id id, 
		   const std::string& name, 
		   t_cvalue cvalue);
void read_values (t_algo_state& state);
void print_solution (t_algo_state& state, const t_activity_set& solution);

t_activity_set sum_to_zero (const t_algo_state& state);
t_cvalue_table_pair pop_first (t_cvalue_table& table);
std::pair<bool, t_activity_set> process_node (const t_cvalue_map& cvalue_map, 
					      t_cvalue_table& table, 
					      const t_cvalue cur_cvalue, 
					      const t_node& cur_node);


int main (int argc, char** argv) {
  if (argc > 1) {
    run_tests();
  }
  else {
    t_algo_state state;
    read_values(state);
    t_activity_set solution = sum_to_zero(state);
    print_solution(state, solution);
  }

  return 0;
}


/*!
DP solver for our problem which progressively scans the DP table for new nodes 
  to process. It does both positive and negative nodes at the same time.
 */
t_activity_set sum_to_zero (const t_algo_state& state) {
  t_cvalue_table table; // DP memoization table.  
  
  // required to bootstrap the positive side.
  table[0] = t_node(true);

  while (!table.empty()) {
    std::pair<t_cvalue, t_node> pair = pop_first(table);
    const t_cvalue cur_cvalue = pair.first;
    const t_node& cur_node = pair.second;

    // required to bootstrap the negative side.
    if (cur_cvalue == 0 && cur_node.is_positive) {
      table [0] = t_node(false);
    }

    std::cerr << std::endl << "CUR Node(" << 
      "cval=" << cur_cvalue << ", " << 
      "dir=" << (cur_node.is_positive ? "+" : "-") << ", " <<
      "act.size=" << cur_node.activities.size() << ")" << std::endl;

    if (cur_node.is_positive) {
      std::pair<bool, t_activity_set> result = 
	process_node(state.plus_map, table, cur_cvalue, cur_node);
      if (result.first) {
	return result.second;
      }
    }

    if (!cur_node.is_positive) {
      std::pair<bool, t_activity_set> result = 
	process_node(state.minus_map, table, cur_cvalue, cur_node);
      if (result.first) {
	return result.second;
      }
    }
  }  

  return t_activity_set();
} 



/*!
Takes a node in the table and creates a series of new permutations by adding a
  new unused value from the provided \c cvalue_map.

If one of the newly created permutations sums up to an existing value and if that
  node has a different is_positive value then the existing node then we have found
  our solution.
 */
std::pair<bool, t_activity_set> process_node (const t_cvalue_map& cvalue_map, 
					t_cvalue_table& table, 
					const t_cvalue cur_cvalue, 
					const t_node& cur_node) 
{
  // For every activity not already done.
  for (t_cvalue_cit cval_it = cvalue_map.begin(); cval_it != cvalue_map.end(); ++cval_it) {
    if (cur_node.activities.find(cval_it->first) != cur_node.activities.end())
      continue;

    // Add a new entry for the sum of that entry plus ours.
    t_node new_node = cur_node;
    new_node.activities.insert(cval_it->first);
    t_cvalue new_cvalue = cur_cvalue + cval_it->second;

    std::cerr << "ADD (val=" << cval_it->second << ") Node(" << 
      "cval=" << new_cvalue << ", " << 
      "dir=" << (new_node.is_positive ? "+" : "-") << ", " <<
      "act.size=" << new_node.activities.size() << ")";

    std::pair<t_cvalue_table::const_iterator, bool> 
      insert_it = table.insert(std::make_pair(new_cvalue, new_node));
	  
    // Did we hit an existing value?
    if (!insert_it.second) {
      std::cerr << " - DUPE";
      // Does it have the oposite sign then ours?
      const t_cvalue_table_pair table_pair = *(insert_it.first);
      if (table_pair.second.is_positive != cur_node.is_positive) {
	// We have our solution!
	std::cerr << " - SOLUTION!" << std::endl;

	t_activity_set solution = table_pair.second.activities;
	solution.insert (new_node.activities.begin(), new_node.activities.end());

	return std::make_pair(true, solution);
      }
    }

    //These are safe to discard because even if the two set don't contain
    //  the same values, the other permutations can still be obtained by taking
    //  different branches in the tree.
    //This also has the nice side effect of greatly reducing the number of nodes
    //  to search.
     
    std::cerr << std::endl;
  }
  return std::make_pair(false, t_activity_set());
}


//! Removes and returns the first value off the table.
t_cvalue_table_pair pop_first (t_cvalue_table& table) {
  t_cvalue_table_pair pair = *table.begin();
  table.erase(table.begin());
  return pair;
}

//! Used to add an entry to the t_algo_state struct.
void add_to_state (t_algo_state& state, 
		   t_activity_id id, 
		   const std::string& name, 
		   t_cvalue cvalue) 
{
  state.name_map[id] = name;

  if (cvalue >= 0)
    state.plus_map[id] = cvalue;
  else
    state.minus_map[id] = cvalue *-1;  

  std::cerr << "ADD Node(" <<
    "id=" << id << ", " <<
    "name=" << name << ", " <<
    "cvalue=" << cvalue << ")" << std::endl;    

}


//! Prints magical rainbows and unicorns also known as the solution.
void print_solution (t_algo_state& state, const t_activity_set& solution) {
  if (solution.size() == 0) {
    std::cout << "no solution" << std::endl;
    return;
  }

  for (t_activity_it act_it = solution.begin(); act_it != solution.end(); ++act_it) {
    std::cout << state.name_map[*act_it] << " ";

    /* The specs states that only the name is printed.
    if (state.plus_map.find(*act_it) != state.plus_map.end()) {
      std::cout << state.plus_map[*act_it];
    }
    else if (state.minus_map.find(*act_it) != state.minus_map.end()) {
      std::cout << "-" << state.minus_map[*act_it];
    }
    */

    std::cout << std::endl;
  }
}


//! Reads the value out of stdin based on the specs provided.
void read_values (t_algo_state& state) {
  int nb_values = 0;
  std::cin >> nb_values;

  for (t_activity_id id = 0; id < nb_values; ++id) {
    // Read the name.
    std::string name;
    char c;
    std::cin >> c;
    while (c != ' ') {
      if (c != '\n' || c != '\r')
	name += c;
      std::cin >> c;
    }
    
    // Read the value.
    t_cvalue cvalue;
    std::cin >> cvalue;

    add_to_state(state, id, name, cvalue);
  }
}



//! generates a name for an activity id.
std::string mkname (int id) {
  std::ostringstream ss; 
  ss << "act_" << id;
  return ss.str();
}

//! Easy way to test the algo.
void run_tests () {

  // Provided examples 

  {
    std::cerr << std::endl << " ******* TEST - Example 1" << std::endl;
    t_algo_state state;
    int id = 0;
    add_to_state(state, ++id, mkname(id), 140);
    add_to_state(state, ++id, mkname(id), 110);
    print_solution(state, sum_to_zero(state));
    std::cerr << "Should be {}" << std::endl;
  }

  {
    std::cerr << std::endl << " ******* TEST - Example 2" << std::endl;
    t_algo_state state;
    int id = -1;
    add_to_state(state, ++id, mkname(id), 802);
    add_to_state(state, ++id, mkname(id), 421);
    add_to_state(state, ++id, mkname(id), 143);
    add_to_state(state, ++id, mkname(id), -302);
    add_to_state(state, ++id, "cookies", 316);
    add_to_state(state, ++id, "mexican-coke", 150);
    add_to_state(state, ++id, mkname(id), -611);
    add_to_state(state, ++id, "coding-six-hours", -466);
    add_to_state(state, ++id, mkname(id), -42);
    add_to_state(state, ++id, mkname(id), -195);
    add_to_state(state, ++id, mkname(id), -295);
    print_solution(state, sum_to_zero(state));
    std::cerr << "Should be {-466, 316, 150}" << std::endl;
  }


  // More test cases that I'm to lazy to construct by hand.

  {
    std::cerr << std::endl << " ******* TEST - Random 1" << std::endl;
    t_algo_state state;

    srand(0);
    for (int id = 0; id < 50; ++id) {
      add_to_state(state, id, mkname(id), rand() %2000 - 1000);
    }
    print_solution(state, sum_to_zero(state));
  }


  {
    std::cerr << std::endl << " ******* TEST - Random 2" << std::endl;
    t_algo_state state;

    srand(1);
    for (int id = 0; id < 10; ++id) {
      add_to_state(state, id, mkname(id), rand() %2000 - 1000);
    }
    print_solution(state, sum_to_zero(state));
  }


}
