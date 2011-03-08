/*!

\author Rémi Attab
\data TBD
\license FreeBSD (see LICENSE file).

 */  

#include <iostream>
#include <sstream>

#include <string>
#include <list>
#include <map>
#include <set>
#include <algorithm>

#include <cstdlib>
#include <cstdio>
#include <cassert>



static const long TIMESTAMP_TOLERANCE = 100;
static const std::string SEP("/");


class t_folder;
class t_file;
struct t_event;

typedef t_folder* p_folder;
typedef t_file* p_file;
typedef t_event* p_event;


typedef std::list<p_folder> t_folder_list;
typedef t_folder_list::iterator t_folder_it;
typedef t_folder_list::const_iterator t_folder_cit;

typedef std::list<p_file> t_file_list;
typedef t_file_list::iterator t_file_it;
typedef t_file_list::const_iterator t_file_cit;

typedef std::list<std::string> t_path;
typedef t_path::iterator t_path_it;
typedef t_path::const_iterator t_path_cit;

typedef std::string t_hash;

typedef std::multimap<t_hash, t_path> t_hash_index;
typedef t_hash_index::value_type t_index_pair;
typedef t_hash_index::iterator t_index_it;
typedef t_hash_index::const_iterator t_index_cit;


template <typename Iterator>
Iterator dec(const Iterator& it) {
  Iterator copy = it;
  copy--;
  return copy;
}

template <typename Iterator>
Iterator inc(const Iterator& it) {
  Iterator copy = it;
  copy++;
  return copy;
}



std::string path_to_string (const t_path& path) {
  std::stringstream ss;

  for(t_path_cit it = path.begin(); it != path.end(); ++it) {
    ss << *it;
    if (*it != SEP && inc(it) != path.end())
      ss << SEP;
  }
  return ss.str();
}


t_path make_path (const std::string& raw_path) {
  t_path path;

  std::size_t base_it = 0;
  while (true) {
    std::size_t next_it = raw_path.find(SEP, base_it);

    if (next_it != base_it) {
      path.push_back(std::string (raw_path, base_it, next_it - base_it));
    }
    // add the root.
    else {
      path.push_back(SEP);
    }

    if (next_it == std::string::npos) break;

    base_it = next_it + 1;
  }

  return path;
}

std::string get_name (const t_path& path) {
  return *path.rbegin();
}

t_path get_parent (const t_path& path) {
  assert (!path.empty());
  return t_path(path.begin(), dec(path.end()));
}





enum t_event_type {
  e_new,
  e_delete,
  e_modify,
  e_move,
  e_copy
};

enum t_file_type {
  e_file,
  e_folder
};



struct t_event { 

  t_file_type file_type;
  t_event_type event_type;
  long timestamp;

  virtual ~t_event() {} 

  virtual void print() = 0;
  std::string get_type_name() {return file_type == e_file ? "file" : "folder";}

protected :

  t_event(t_file_type f, t_event_type ev, long ts) : 
    file_type(f), event_type(ev), timestamp(ts) 
  {}

};


struct t_event_set_comp : public std::binary_function<p_event, p_event, bool> {
  bool operator() (const p_event lhs, const p_event rhs) const {
    return lhs->timestamp < rhs->timestamp;
  }
};

typedef std::set<p_event, t_event_set_comp> t_event_list;
typedef t_event_list::iterator t_event_it;
typedef t_event_list::const_iterator t_event_cit;




struct t_event_new : public t_event { 
  t_path path;
  t_hash hash;

  t_event_new(t_file_type f, long ts, const t_path& p, t_hash h = "") : 
    t_event::t_event(f, e_new, ts), path(p), hash(h)
  {}

  virtual void print() {
    if (file_type ==  e_file) {
      printf("Created the %s \"%s\" in the folder \"%s\" with the hash value \"%s\".\n",
	     get_type_name().c_str(), 
	     get_name(path).c_str(), 
	     path_to_string(get_parent(path)).c_str(),
	     hash.c_str());
    }

    else {
      printf("Created the %s \"%s\" in the folder \"%s\".\n",
	     get_type_name().c_str(), 
	     get_name(path).c_str(), 
	     path_to_string(get_parent(path)).c_str(),
	     hash.c_str());   
    }
  }
};

struct t_event_delete : public t_event { 
  t_path path;
  t_hash hash;

  t_event_delete(t_file_type f, long ts, const t_path& p, const t_hash& h) : 
    t_event::t_event(f, e_delete, ts), path(p), hash(h)
  {}

  virtual void print() {
    printf("Deleted the %s \"%s\" in the folder \"%s\".\n",
	   get_type_name().c_str(), 
	   get_name(path).c_str(), 
	   path_to_string(get_parent(path)).c_str());
  }
};

struct t_event_modify : public t_event {
  t_path path;
  t_hash old_hash;
  t_hash new_hash;

  t_event_modify(t_file_type f, 
		 long ts,
		 const t_path& p, 
		 const t_hash& old_h, 
		 const t_hash& new_h) : 
    t_event::t_event(f, e_modify, ts), path(p), old_hash(old_h), new_hash(new_h) 
  {}
  virtual void print() {
    printf("Modified the %s \"%s\" in the folder \"%s\". The new hash value is \"%s\".\n",
	   get_type_name().c_str(), 
	   get_name(path).c_str(), 
	   path_to_string(get_parent(path)).c_str(), 
	   new_hash.c_str());
  }
};

struct t_event_move : public t_event {
  t_path old_path;
  t_path new_path;

  t_event_move(t_file_type f, long ts, const t_path& old_p, const t_path& new_p):
    t_event::t_event(f, e_move, ts), old_path(old_p), new_path(new_p)
  {}

  bool is_rename() const { return get_name(old_path) != get_name(new_path);}
  bool is_move() const { return get_parent(old_path) != get_parent(new_path);}

  virtual void print() {
      if (is_rename() && is_move()) {
      printf("Moved the %s \"%s\" in the folder \"%s\" to the folder \"%s\" with the name \"%s\".\n",
	     get_type_name().c_str(), 
	     get_name(old_path).c_str(), 
	     path_to_string(get_parent(old_path)).c_str(), 
	     path_to_string(get_parent(new_path)).c_str(),
	     get_name(new_path).c_str());
    }

    else if (is_rename()) {
      printf("Renamed the %s \"%s\" in the folder \"%s\" to \"%s\".\n",
	     get_type_name().c_str(), 
	     get_name(old_path).c_str(), 
	     path_to_string(get_parent(old_path)).c_str(), 
	     get_name(new_path).c_str());
    }

    else if (is_move()) {
      printf("Moved the %s \"%s\" in the folder \"%s\" to the folder \"%s\".\n",
	     get_type_name().c_str(), 
	     get_name(old_path).c_str(), 
	     path_to_string(get_parent(old_path)).c_str(), 
	     path_to_string(get_parent(new_path)).c_str());
    }
    
    else {  
      std::cout << "(" << is_rename() << ", " << is_move() << ") " <<
	"( " <<  path_to_string(old_path) << ", " << path_to_string(new_path) << ")" <<
	std::endl;
    }

  }

};

struct t_event_copy : public t_event {
  t_path src_path;
  t_path dest_path;
  
  t_event_copy(t_file_type f, long ts, const t_path& src_p, const t_path& dest_p) :
    t_event::t_event(f, e_copy, ts), src_path(src_p), dest_path(dest_p)
  {}
  virtual void print() {
    printf("Copied the %s \"%s\" from the folder \"%s\" to the folder \"%s\" with the name \"%s\".\n",
	   get_type_name().c_str(), 
	   get_name(src_path).c_str(), 
	   path_to_string(get_parent(src_path)).c_str(), 
	   path_to_string(get_parent(dest_path)).c_str(),
	   get_name(dest_path).c_str());
  }
};




class t_algo_state {

  // Equivalent of boost::noncopyable.
  t_algo_state(const t_algo_state& src) {}
  bool operator= (const t_algo_state& src) {}

public :
  t_algo_state () : hash_index(), events() {}
  
  t_hash_index hash_index;
  t_event_list events;

};




void add_to_state (t_algo_state& state, p_event ev);
void simplify_state (t_algo_state& state);
void print_state (t_algo_state& state);

void read_events (t_algo_state& state);
void run_tests ();

int main (int argc, char** argv) {
  if (argc > 1) {
    run_tests();
  }
  else {
    t_algo_state state;
    read_events (state);
    simplify_state (state);
    print_state (state);
  }
  return 0;
}





// Will delete the event at the iterator so don't use it afterwards.
t_event_it remove_event (t_algo_state& state, t_event_it it) {
  p_event ev = *it;
  t_event_it next_it = inc(it);

  std::cout << "REM - "; ev->print();

  state.events.erase(it);
  delete ev; // Should really be handled by boost::shared_ptr...

  return next_it;
}

void remove_event (t_algo_state& state, t_event_it start, t_event_it end) {
  t_event_it it = start;
  while (it != end) {
    it = remove_event(state, it);
  }
}


void print_index (t_algo_state& state) {
  std::cout << "I== - ";
  for (t_index_it it = state.hash_index.begin(); it != state.hash_index.end(); ++it) {
    std:: cout << "(" << it->first << ", " << path_to_string(it->second) << ") ";
  }
  std::cout << std::endl;
 
}

void remove_from_index (t_algo_state& state, const t_hash& key, const t_path& value){
  const t_index_it start = state.hash_index.lower_bound(key);
  const t_index_it end = state.hash_index.upper_bound(key);
  assert(start != state.hash_index.end());

  for (t_index_it it = start; it != end; it++) {
    if (it->second == value) {
      state.hash_index.erase(it);
      break;
    }
  }
}


bool simplify_to_move_event(t_algo_state& state, p_event ev, t_event_it prev_ev_it) {
  p_event prev_ev = *prev_ev_it;

  if (ev->timestamp - prev_ev->timestamp > TIMESTAMP_TOLERANCE)
    return false;
  
  if (prev_ev->file_type == e_file && ev->event_type == e_new) {
    const t_event_new* new_ev = static_cast<t_event_new*> (ev);

    if (prev_ev->event_type == e_delete) {
      const t_event_delete* prev_del_ev = static_cast<t_event_delete*>(prev_ev);

      if (prev_del_ev->hash == new_ev->hash) {
	p_event move_ev = 
	  new t_event_move(e_file, ev->timestamp, prev_del_ev->path, new_ev->path);
	state.events.insert(move_ev);

	std::cout << "MOV - "; move_ev->print();

	remove_event(state, prev_ev_it);
	prev_ev_it = state.events.end();
	prev_ev = NULL;

	return true;
      }
    }
  }

  return false;
}

bool simplify_to_modify_event(t_algo_state& state, p_event ev, t_event_it prev_ev_it) {
  p_event prev_ev = *prev_ev_it;

  if (ev->timestamp - prev_ev->timestamp > TIMESTAMP_TOLERANCE)
    return false;
  
  if (prev_ev->file_type == e_file && ev->event_type == e_new) {
    const t_event_new* new_ev = static_cast<t_event_new*> (ev);

    if (prev_ev->event_type == e_delete) {
      const t_event_delete* prev_del_ev = static_cast<t_event_delete*>(prev_ev);

      if  (prev_del_ev->path == new_ev->path) {
	p_event modify_ev = 
	  new t_event_modify(e_file, ev->timestamp, new_ev->path, prev_del_ev->hash, new_ev->hash);
	state.events.insert(modify_ev);

	std::cout << "MOD - "; modify_ev->print();

	remove_event(state, prev_ev_it);
	prev_ev_it = state.events.end();
	prev_ev = NULL;

	return true;
      }
    }
  }

  return false;

}

bool simplify_to_copy_event(t_algo_state& state, p_event ev) {
  if (ev->event_type == e_new) {
    const t_event_new* new_ev = static_cast<t_event_new*> (ev);

    t_index_it index_it = state.hash_index.upper_bound(new_ev->hash);
    if (index_it != state.hash_index.end()) {

      p_event copy_ev = 
	new t_event_copy(e_file, ev->timestamp, index_it->second, new_ev->path);
      state.events.insert(copy_ev);
      std::cout << "COP - "; copy_ev->print();	  

      return true;
    }   
  }

  return false;
}


void add_to_state (t_algo_state& state, p_event ev) {
  // We process folder events later to avoid mix ups with file events.
  if (ev->file_type == e_folder) {
    state.events.insert(ev);
    return;
  }

  bool is_added = false;

  // Try to simplify to an higher level event.

  if (!state.events.empty()) {
    t_event_it prev_ev_it = dec(state.events.end());
    p_event prev_ev = *prev_ev_it;

    if (!is_added) is_added = simplify_to_move_event(state, ev, prev_ev_it);
    if (!is_added) is_added = simplify_to_modify_event(state, ev, prev_ev_it);

  }

  if (!is_added) is_added = simplify_to_copy_event(state, ev);

  // Unable to simplify so just add the event as is.
  if (!is_added) {
    std::cout << "ADD - "; ev->print();
    state.events.insert(ev);
  }


  // Update the hash index.
  if (ev->file_type == e_file) {

    // No matter what happened above, if a new event comes in for a file
    //   Then something changed so we need to update the index.
    if (ev->event_type == e_new) {
      const t_event_new* new_ev = static_cast<t_event_new*> (ev);

      std::cout << "I++ - (" << new_ev->hash << ", " << path_to_string(new_ev->path) << ")" << std::endl;

      state.hash_index.insert(std::make_pair(new_ev->hash, new_ev->path));
    }

    else if (ev->event_type == e_delete) {
      const t_event_delete* del_ev = static_cast<t_event_delete*> (ev);

      std::cout << "I-- - (" << del_ev->hash << ", " << path_to_string(del_ev->path) << ")" << std::endl;

      remove_from_index(state, del_ev->hash, del_ev->path);
    }
  }

}




void simplify_state (t_algo_state& state) {

}



void print_state (t_algo_state& state) {
  for (t_event_it it = state.events.begin(); it != state.events.end(); ++it) {
    (*it)->print();
  }
}




void run_tests() {

  {
    t_algo_state s;
    long ts = 0;

    add_to_state(s, new t_event_new(e_folder, ++ts, make_path("/a")));
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/b.t"), "1111"));
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/c.t"), "2222"));
    
    // Rename
    add_to_state(s, new t_event_delete(e_file, ++ts, make_path("/a/c.t"), "2222"));
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/d.t"), "2222"));
    
    // Modify
    add_to_state(s, new t_event_delete(e_file, ++ts, make_path("/a/b.t"), "1111"));
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/b.t"), "1112"));

    add_to_state(s, new t_event_new(e_folder, ++ts, make_path("/a/e")));

    // Copy of /a/b.t
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/e/f.t"), "1112"));    

    // Move and Rename
    add_to_state(s, new t_event_delete(e_file, ++ts, make_path("/a/b.t"), "1112"));    
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/e/g.t"), "1112"));    

    // To slow for a Move
    add_to_state(s, new t_event_delete(e_file, ++ts, make_path("/a/e/g.t"), "1112"));    
    ts += TIMESTAMP_TOLERANCE +1;
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/h.t"), "1112"));    

    // To slow for a Modify
    add_to_state(s, new t_event_delete(e_file, ++ts, make_path("/a/e/d.t"), "2222"));    
    ts += TIMESTAMP_TOLERANCE +1;
    add_to_state(s, new t_event_new(e_file, ++ts, make_path("/a/e/d.t"), "2223"));    

    simplify_state(s);

    std::cout << std::endl;
    print_state(s);
  }

}



void read_events (t_algo_state& state) {
  static const std::string ADD_EV = "ADD";
  static const std::string DEL_EV = "DEL";
  static const std::string NULL_HASH = "-";


  int nb_events;
  std::cin >> nb_events;
  
  for (int i = 0; i < nb_events; ++i) {
    char buffer [255];

    std::string ev_name;
    std::cin >> ev_name;

    long timestamp;
    std::cin >> timestamp;

    std::string raw_path;
    std::cin >> raw_path;
    t_path path = make_path(raw_path);

    t_hash hash;
    std::cin >> hash;

    t_file_type file_type = hash == NULL_HASH ? e_folder : e_file;

    p_event ev;
    if (ev_name == ADD_EV)
      ev = new t_event_new (file_type, timestamp, path, hash);
    else if (ev_name == DEL_EV)
      ev = new t_event_delete (file_type, timestamp, path, hash);
    else
      assert(false && "Unknown event");

    ev->print();

    add_to_state(state, ev);
  }
}
