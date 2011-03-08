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

typedef std::map<std::string, p_file> t_hash_index;
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

    base_it = next_it;
    base_it++;
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


struct t_root_folder {} root_folder;

class t_folder {
  std::string m_name;
  p_folder m_parent;

  t_folder_list m_child_folders;
  t_file_list m_child_files;
  t_event_list m_events;

  // Equivalent of boost::noncopyable.
  t_folder() {}
  t_folder(const t_folder& src) {}
  bool operator= (const t_folder& src) {}


public:

  t_folder(const t_root_folder& root_folder) :
    m_name(SEP), m_parent(NULL),
    m_child_folders(), m_child_files(), m_events()
  {}

  t_folder(const std::string& name, p_folder parent) :
    m_name(name), m_parent(parent),
    m_child_folders(), m_child_files(), m_events()
  {
    assert(m_parent);
  }

  ~t_folder() {
    std::for_each(m_child_folders.begin(), m_child_folders.end(), delete_folder);
    std::for_each(m_child_files.begin(), m_child_files.end(), delete_file);
    std::for_each(m_events.begin(), m_events.end(), delete_event);
  }

  std::string name() const {return m_name;}
  t_path path() const {
    t_path cur_path;
    path(cur_path);
    return cur_path;
  }


  bool has_parent() const {return m_parent != NULL;}
  m_parent parent() const {return m_parent;}

  t_folder_list& child_folders() {return m_child_folders;}
  const t_folder_list& child_folders() const {return m_child_folders;}

  t_file_list& child_files() {return m_child_files;}
  const t_file_list& child_files() const {return m_child_files;}

  t_event_list& events() {return m_events;}
  const t_event_list& events() const {return m_events;}

  p_folder find_folder(t_path_cit start, t_path_cit end) const {
    if (start == end)
      return this;

    t_folder_name_comp comp (*start);
    t_folder_cit folder_it = 
      std::find_if (m_child_folders.begin(), m_child_folders.end(), comp);
    
    if (folder_it == m_child_folders.end())
      return NULL;

    t_path_cit next = start;
    next++;
    return (*folder_it)->find_folder(next, end);
  }


  p_file find_file(t_path_cit start, t_path_cit end) const {
    assert(start != end);

    t_path_cit filename_it = end;
    filename_it--;

    p_folder parent_folder = find_folder(start, filename_it);
    if (parent_folder == NULL)
      return NULL;
    
    t_file_name_comp comp (*filename_it);
    t_file_cit file_it = std::find_if (m_child_files.begin(), m_child_files.end(), comp);
    if (file_it == m_child_files.end())
      return NULL;

    return *file_it;
  }


private : 

  void path(t_path& cur_path) const {
    if (m_parent) {
      m_parent->path(cur_path);
    }
    cur_path.push_back(m_name);
  }

};


class t_file {

  std::string m_name;
  std::string m_hash;
  p_folder m_parent;
  t_event_list m_events;

  // Equivalent of boost::noncopyable.
  t_file() {}
  t_file(const t_file& src) {}
  bool operator= (const t_file& src) {}


public :  
  
  t_file(const std::string& name, const std::string& hash, p_folder parent) :
    m_name(name), m_hash(hash), m_parent(parent), m_events()
  {
    assert(m_parent);
  }

  ~t_file() {
    std::for_each(m_events.begin(), m_events.end(), delete_event);
  }

  std::string name() const {return m_name;}
  t_path path() const {
    t_path cur_path = m_parent->path();
    cur_path.push_back(m_name);
    return cur_path;
  }
 
  p_folder parent() const {return m_parent;}

  t_event_list& events() {return m_events;}
  const t_event_list& () const {return m_events;}

};


class t_algo_state {
  // Equivalent of boost::noncopyable.
  t_file() {}
  t_file(const t_file& src) {}
  bool operator= (const t_file& src) {}

public :
  
  t_folder root (root_folder);
  t_hash_index hash_index;

};


enum t_event_type {
  e_new,
  e_delete,
  e_modify,
  e_move,
  e_copy
};

struct t_event { 
  t_event_type type;
  t_event(t_event_type t) : type(t) {}

  virtual ~t_event() {} 
};

struct t_event_new : public t_event { 
  t_event_new() : t_event::t_event(e_new) {}
};

struct t_event_delete : public t_event { 
  t_event_delete() : t_event::t_event(e_delete) {}
};

struct t_event_modify : public t_event {
  std::string new_hash;
  t_event_modify(const std::string& hash) : 
    t_event::t_event(e_modify), new_hash(hash) 
  {}
};

struct t_event_move : public t_event {
  t_path old_path;
  t_event_move(const t_path& path):
    t_event::t_event(e_move), old_path(path)
  {}
};

struct t_event_copy : public t_event {
  t_path old_path;
  t_event_copy(const t_path& path) :
    t_event::t_event(e_copy), old_path(path)
  {}
};



void add_file_to_state (t_algo_state& state, const t_event& ev, const& t_path path);
void add_folder_to_state (t_algo_state& state, const t_event& ev, const& t_path path, std::string hash);
void simplify_state (t_algo_state& state);

void read_events (t_algo_state& state);
void run_tests ();

int main (int argc, char** argv) {
  

}



