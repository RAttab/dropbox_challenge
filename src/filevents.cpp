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
#include <algorithm>

#include <cstdlib>
#include <cassert>



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

typedef std::list<p_event> t_event_list;
typedef t_event_list::iterator t_event_it;
typedef t_event_list::const_iterator t_event_cit;

typedef std::list<std::string> t_path;
typedef t_path::iterator t_path_it;
typedef t_path::const_iterator t_path_cit;

typedef std::string t_hash;

typedef std::multimap<t_hash, t_path> t_hash_index;
typedef t_hash_index::value_type t_index_pair;
typedef t_hash_index::iterator t_index_it;
typedef t_hash_index::const_iterator t_index_cit;



struct t_delete_folder : public std::unary_function<p_folder, void> { 
  void operator() const (p_folder folder) {delete folder;}
} delete_folder;
struct t_delete_file : public std::unary_function<p_file, void> {
  void operator() const (p_folder file) {delete file;}
} delete_file;
struct t_delete_event : public std::unary_function<p_event, void> {
  void operator() const (p_event event) {delete event;}
} delete_event;



// I find the stl bind function ugly to use so this is better.
struct t_folder_name_comp : public std::unary_function<p_folder, bool> {
  const std::string m_name;
  t_folder_name_comp (const std::string name) : m_name (name) {}
  bool operator() const (p_folder folder) {return folder->name() == m_name;}
};
struct t_file_name_comp : public std::unary_function<p_file, bool> {
  const std::string m_name;
  t_file_name_comp (const std::string name) : m_name (name) {}
  bool operator() const (p_file file) {return file->name() == m_name;}
};


template<Iterator>
Iterator dec(const Iterator& it) {
  Iterator copy = it;
  copy--;
  return copy;
}

template <Iterator>
Iterator inc(const Iterator& it) {
  Iterator copy = it;
  copy++;
  return copy;
}


t_path make_path (const std::string& raw_path) {
  typedef std::string::const_iterator t_cit;

  t_path path;

  t_cit base_it = raw_path.begin();
  while (true) {
    t_cit next_it = std::find(base_it, raw_path.end(), SEP);

    if (next_it == raw_path.end()) break;

    if (next_it != base_it) {
      path.push_back(std::string (base_it, next_it));
    }
    // add the root.
    else {
      path.push_back(SEP);
    }

    base_it = inc(next_it);
  }

  return path;
}

std::string path_to_string (const t_path& path) {
  std::stringstream ss;

  for(t_path_cit it = path.begin(); it != path.end(); ++it) {
    ss << *it;
    if (*it != SEP)
      ss << SEP;
  }
  return ss.str();
}

std::string get_name (const t_path& path) {
  return *path.rbegin();
}

t_path get_parent (const t_path& path) {
  assert (!path.empty());
  return t_path(path.begin(), dec(path.end()));
}




class t_algo_state {

  // Equivalent of boost::noncopyable.
  t_algo_state() {}
  t_algo_state(const t_algo_state& src) {}
  bool operator= (const t_algo_state& src) {}

public :
  
  t_hash_index hash_index;
  t_event_list events;

};


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

  t_event(t_file_type f, t_event_type ev) : 
    file_type(f), event_type(ev) 
  {}

  virtual ~t_event() {} 
};


struct t_event_new : public t_event { 
  t_path path;
  t_hash hash;
  t_event_new(t_file_type f, const t_path& p, t_hash h = "") : 
    t_event::t_event(f, e_new), path(p), hash(h)
  {}
};

struct t_event_delete : public t_event { 
  t_path path;
  t_hash hash;
  t_event_delete(t_file_type f, const t_path& p, const t_hash& h) : 
    t_event::t_event(f, e_delete), path(p), hash(h)
  {}
};

struct t_event_modify : public t_event {
  t_path path;
  t_hash old_hash;
  t_hash new_hash;

  t_event_modify(t_file_type f, 
		 const t_path& p, 
		 const t_hash& old_h, 
		 const t_hash& new_h) : 
    t_event::t_event(f, e_modify), path(p), old_hash(old_h), new_hash(new_h) 
  {}
};

struct t_event_move : public t_event {
  t_path old_path;
  t_path new_path;

  t_event_move(t_file_type f, const t_path& old_p, const t_path& new_p)
    t_event::t_event(e_move), old_path(old_p), new_path(new_p)
  {}
  bool is_rename() const { return get_name(old_path) != get_name(new_path);}
  bool is_move() const { return get_parent(old_path) != get_parent(new_path);}
};

struct t_event_copy : public t_event {
  t_path src_path;
  t_path dest_path;
  
  t_event_copy(t_file_type f, const t_path& src_p, const t_path& dest_p) :
    t_event::t_event(f, e_copy), src_path(src_p), dest_path(dest_p)
  {}
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




void remove_event (t_aglo_state& state, t_event_it start, t_event_it end) {
  t_event_it it = start;
  while (it != end) {
    it = remove_event(state, it);
  }
}

// Will delete the event at the iterator so don't use it afterwards.
t_event_it remove_event (t_algo_state& state, t_event_it it) {
  p_event ev = *it;
  t_event_it next_it = state.events.erase(it);
  delete ev; // Should really be handled by boost::shared_ptr...
  return next_it;
}


void remove_from_index (t_algo_state& state, const t_hash& key, const t_path& value){
  const t_index_it start = state.hash_index.upper_bound(key);
  const t_index_it end = state.hash_index.lower_bound(key);
  assert(start != state.hash_index.end());

  for (t_index_it it = start; it != end; it++) {
    if (it->second == value) {
      state.hash_index.erase(it);
      break;
    }
  }
}

void add_to_state (t_algo_state& state, p_event ev) {
  // We process folder events later to avoid mix ups with file events.
  if (ev->file_type == e_folder) {
    state.events.push_back(ev);
    return;
  }
  
  t_event_it prev_ev_it = state.event.rbegin();
  p_event prev_ev = *prev_ev_it;

  if (prev_ev->file_type == e_file && ev->event_type == e_new) {
    const t_event_new* new_ev = static_cast<t_event_new*> (ev);

    if (prev_ev->event_type == e_delete) {
      const t_event_delete* prev_del_ev = static_cast<t_event_delete*>(prev_ev);

      // Move event
      if (prev_del_ev->hash == new_ev->hash) {
	p_event move_ev = new t_event_move(e_file, prev_del_ev->path, new_ev->path);
	state.events.push_back(move_ev);

	remove_event(state, prev_ev_it);
	prev_ev_it = state.event.end();
	prev_ev = NULL;
      }

      // Modify event
      else if (prev_del_ev->path == new_ev->path) {
	p_event modify_ev = 
	  new t_event_modify(e_file, new_ev->path, prev_del_ev->hash, new_ev->hash);
	state.events.push_back(modify_ev);

	remove_event(state, prev_ev_it);
	prev_ev_it = state.event.end();
	prev_ev = NULL;
      }

    }
    else {
      t_hash_it hash_it = state.hash_index.find(new_ev->hash);

      //Copy event
      if (hash_it != state.hash_index.end()) {
	p_event copy_ev = new t_event_copy(e_file, hash_it->second, new_ev->path);
      }
      
    }    
    
  }
  // Nothing special, just add the event.
  else {
    state.events.push_back(ev);
  }

  if (ev->file_type = e_file) {

    // No matter what happened above, if a new event comes in for a file
    //   Then something changed so we need to update the index.
    if (ev->event_type == e_new) {
      const t_event_new* new_ev = static_cast<t_event_new*> (ev);
      state.hash_index[new_ev->hash] = new_ev->path;
    }

    else if (ev->event_type == e_delete) {
      const t_event_delete* del_ev = static_cast<t_event_delete*> (ev);
      remove_from_index(state, del_ev->hash, del_ev->path);
    }
  }

}








std::string read_token () {
  char c;
  std::cin >> c;
  
  std::string buf;

  while (c != ' ' || c != '\n') {
    buf += c;
    std::cin >> c;
  };

  return buf;
}


void read_events (t_algo_state& state) {
  static const std::string ADD_EV = "ADD";
  static const std::string DEL_EV = "DEL";
  static const std::string NULL_HASH = "-";


  int nb_events;
  std::cin >> nb_events;

  for (int i = 0; i < nb_events; ++i) {

    std::string ev = read_token();
    t_path path = make_path(read_token());
    std::string hash = read_token();

    t_file_type file_type = hash == NULL_HASH ? e_folder : e_file;

    p_event ev;
    if (ev == ADD_EV)
      ev = new t_event_new (file_type, path, hash);
    else if (ev == DEL_EV)
      ev = new t_event_delete (file_type, path, hash);
    else
      assert(false || "Unknown event");

    add_to_state(state, ev);
  }
}
