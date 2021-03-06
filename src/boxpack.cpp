
/*!
\author Rémi Attab
\date 5/03/2011
\license FreeBSD (see LICENSE file).

This file contains the solution to the first Dropbox challenge.

How it all works:

First of all the tallest block is placed and will serve as the maximum height 
of our bin. After that we proceed in two steps:

  1) Append the tallest block at the end of the bin.
  2) Look for any free spaces in the bin where we could add a box and keep 
     doing so until there's no more space.

Step 1) is a straight forward greedy algorithm. This is also the only step
that influences the size of the bin.

Step 2) uses what I call a free list to index the free space available.
The free list is made up of free boxes which are just like regular boxes
except that their width is defined by the size of the bin. So basically
they stretch as the bin stretchs from some x to the edge of the bin.

To place a box we first iterate through every element of the free list
and every boxes to find the biggest box that will fit in one of our free
boxes. The box is then placed along the top left corner of the free box.
The free box is then split in two (under and to the right). Finally, we
adjust any other free boxes that might overlap with the new box.

We repeat this process until no more boxes can be placed and then it's
back to step 1) till there's no more boxes to place.

Asymptoticly, this algo is O(n^3) but since the free list is kept
pretty small, it's probably closer to O(n^2). Not that great but better
then the O(2^n) naive algo and much nicer results then the various
greedy algos.

Note that this algorithm could be improved further by using unbounded 
height as well as width for the free boxes. This would allow us to
grow our bin in both dimensions but would also require an heuristic
to determine when to grow horrizontally. This would allow us to also
expand our solution to other problem type like texture packing.
 */


/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <iostream>  

#include <set>
#include <list>
#include <vector>
#include <algorithm>

#include <cstdlib>
#include <cstdio>


/*******************************************************************************
 * struct t_box
 ******************************************************************************/

//! Represents a box in the bin (either an actual or a free box).
struct t_box {
  t_box () : width(0), height(0), x(0), y(0) {}
  t_box (int w, int h) : width(w), height(h), x(0), y(0) {}
  int width;
  int height;
  int x, y;
  int area () const {return width*height;}
  int top () const {return y + height;}
  int right () const {return x + width;}
  void print () const {
    std::cerr << "Box(" << width << ", " << height << ") -> " << x << ", " << y << std::endl;
  }
};


/*******************************************************************************
 * Typedefs
 ******************************************************************************/

typedef std::list<t_box> t_box_list;
typedef t_box_list::iterator t_box_it;
typedef t_box_list::const_iterator t_box_cit;


/*******************************************************************************
 * t_box utilities
 ******************************************************************************/

//! Sorts by placing tallest boxes first.
struct t_box_ref_height_comp : 
  public std::binary_function<t_box_it, t_box_it, bool>
{
  bool operator() (const t_box_it& lhs, const t_box_it& rhs) const {
    if (lhs->height == rhs->height) 
      return lhs->area() > rhs->area();
    return lhs->height > rhs->height;
  }
} box_ref_height_comp;


//! List of boxes to add ordered by tallest to smallest.
typedef std::multiset<t_box_it, t_box_ref_height_comp> t_box_ref_list;
typedef t_box_ref_list::iterator t_box_ref_it;


//! Orders by box position
struct t_box_pos_comp :
  public std::binary_function<t_box, t_box, bool>
{
  bool operator() (const t_box& lhs, const t_box& rhs) const {
    if (lhs.x != rhs.x)
      return lhs.x < rhs.x;
    return lhs.y < rhs.y;
  }
} box_pos_comp;


//! Ordered by position list of free box (no duplicates).
typedef std::set<t_box, t_box_pos_comp> t_free_list;
typedef t_free_list::iterator t_free_it;


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

bool read_boxes (t_box_list& out_list);
t_box pack_boxes (t_box_list& box_list);
void print_boxes (const t_box_list& box_list, const t_box& bin);

void run_tests();
void run_packer(t_box_list& list);

int min (int a, int b) {return a < b ? a : b;}
int max (int a, int b) {return a > b ? a : b;}


/*******************************************************************************
 * Entry Point
 ******************************************************************************/

//! Entry point. Command line defines whether we do tests or user inputs.
int main (int argc, char** argv) {

  if (argc > 1) {
    run_tests();
  }
  else {
    t_box_list box_list;
    if (!read_boxes(box_list)) {
      std::cerr << "Unable to read the box list!" << std::endl;
      exit(1);
    }
    run_packer(box_list);
  }
  return 0;
}


/*******************************************************************************
 * Main solver.
 ******************************************************************************/

void place_first_box (t_box_ref_list& box_queue, t_box& bin);
void place_box_greedy (t_box& new_box, t_box& bin, t_free_list& free_list);
void place_box_free_list (t_box_ref_list& box_queue, t_free_list& free_list, const t_box& bin);
void extend_bin (t_box& bin, const t_box& new_box);


//! Main loop of the algorithm. Nothing too fancy so just read it.
t_box pack_boxes (t_box_list& box_list) {
  
  t_box_ref_list box_queue;
  for (t_box_it it = box_list.begin(); it != box_list.end(); ++it) {
    box_queue.insert(it);
  }

  t_box bin;
  t_free_list free_list;

  place_first_box(box_queue, bin);

  while (box_queue.size() > 0) {
    t_box_it first_box = *(box_queue.begin());
    place_box_greedy(*first_box, bin, free_list);
    box_queue.erase(box_queue.begin());
    
    place_box_free_list(box_queue, free_list, bin);
  }

  return bin;
}


//! Extends the bin to fit the new box.
void extend_bin (t_box& bin, const t_box& new_box) {
  bin.width = max(bin.width, new_box.right());
  bin.height = max(bin.height, new_box.top());
}


/*******************************************************************************
 * Greedy solver
 ******************************************************************************/

//! The first box defines the height of the bin so we treat it specially.
void place_first_box (t_box_ref_list& box_queue, t_box& bin) {
  t_box_ref_it first_ref = box_queue.begin();
  t_box_it first_box = *first_ref;

  extend_bin(bin, *first_box);
  first_box->x = first_box->y = 0;

  std::cerr << "1 ";
  first_box->print();

  box_queue.erase(first_ref);
}


//! Places the tallest box at the end of the bin and updates the free list accordingly.
void place_box_greedy (t_box& new_box, t_box& bin, t_free_list& free_list) {
  new_box.x = bin.width;
  new_box.y = 0;
  extend_bin (bin, new_box);

  std::cerr << "G ";
  new_box.print();

  t_box free_box;
  free_box.x = new_box.x;
  free_box.y = new_box.top();
  free_box.height = bin.height - new_box.height;
  if (free_box.height > 0) 
    free_list.insert(free_box);
}


/*******************************************************************************
 * Free list solver.
 ******************************************************************************/

std::pair<t_free_it, t_box_ref_it> 
free_list_search (t_box_ref_list& box_queue, t_free_list& free_list, const t_box& bin);
void free_list_update (t_free_it free_it, 
		       const t_box_it& queue_box, 
		       t_free_list& free_list, 
		       const t_box& bin);
void set_free_height (t_free_it free_it, t_free_list& free_list, int height);
void set_free_y (t_free_it free_it, t_free_list& free_list, int new_y);
bool is_free_redundant (t_free_list& free_list, const t_box& new_free);


//! Places the biggest possible boxes in the available free list entries.
void place_box_free_list (t_box_ref_list& box_queue, 
			  t_free_list& free_list,
			  const t_box& bin) 
{

  while (true) {
    
    /*
      std::cerr << std::endl << "F LIST (" << free_list.size() << ")" << std::endl;
      for (t_free_it it = free_list.begin(); it != free_list.end(); it++) {
      std::cerr << "\t";
      it->print(); 
      }
    */

    std::pair<t_free_it, t_box_ref_it> result = 
      free_list_search(box_queue, free_list, bin);
    
    const t_free_it free_it = result.first;
    const t_box_ref_it queue_it = result.second;
    if (free_it == free_list.end())
      return;

    // Place the new box along the the top (rotate as needed).
    const t_box& old_free = *free_it;
    const t_box_it& queue_box = *queue_it;

    if (queue_box->height > old_free.height) {
      std::swap(queue_box->height, queue_box->width);
    }

    queue_box->x = old_free.x;
    queue_box->y = old_free.top() - queue_box->height;
    box_queue.erase(queue_it);

    std::cerr << "F ";
    queue_box->print();
    std::cerr << "\tfrom Free";  
    old_free.print();

    // Update the free box list.
    free_list_update(free_it, queue_box, free_list, bin);
  }
}


/*!
  Find the biggest box we can shove in a free spot (if any).
  This is the slowest spot of our algorithm. Lots of stuff to check.
  Luckily, free_list usually remains pretty small so it's not as bad as it looks.
*/
std::pair<t_free_it, t_box_ref_it> 
free_list_search (t_box_ref_list& box_queue, t_free_list& free_list, const t_box& bin) {

  int max_area = -1;
  t_free_it found_free = free_list.end();
  t_box_ref_it found_box = box_queue.end();
  

  for (t_free_it free_it = free_list.begin(); free_it != free_list.end(); ++free_it) {
    int free_width = bin.width - free_it->x;

    for (t_box_ref_it queue_it = box_queue.begin(); queue_it != box_queue.end(); ++queue_it) {
      const t_box_it& queue_box = *queue_it;
      
      // Is it worth continuing?
      if (queue_box->area() <= max_area) 
	continue;
      
      // Can we fit it in?
      if (queue_box->height > max(free_width, free_it->height))
	continue;
      if (queue_box->width > min(free_width, free_it->height))
	continue;

      max_area = queue_box->area();
      found_free = free_it;
      found_box = queue_it;
    }
  }

  return std::make_pair(found_free, found_box);
}


//! Updates the free list to take into account the added block.
void free_list_update (t_free_it free_it, 
		       const t_box_it& queue_box, 
		       t_free_list& free_list, 
		       const t_box& bin) 
{
  t_free_it old_free = free_it;
  int new_free_x = queue_box->right();
 
  int old_y = old_free->y;
  int old_height = old_free->height;
 
  // Update the entries.
  //  Trim the free blocks so that they don't overlap our new block.
  for (t_free_it it = free_list.begin(); it != free_list.end(); ++it) {
    
    // If the free block apears after then it can't overlap anything.
    if (it->x >= new_free_x)
      break;

    int height_diff = it->top() - queue_box->y;
    int y_diff = queue_box->top() - it->y;

    // A block is overlapping the bottom of the free block so trim the bottom.
    if (y_diff > 0 && queue_box->top() < it->top()) {
      set_free_y(it, free_list, it->y + y_diff);
    }
    // A block is overlapping the top of a the free block so trim the top.
    else if (height_diff > 0 && queue_box->y >= it->y) {
      set_free_height(it, free_list, it->height - height_diff);
    }
    // The block is overlapping the entire free block, get rid of it.
    else if (y_diff > 0 && height_diff > 0) {
      set_free_height(it, free_list, 0);
    }
  }

  // Create the new free box on the right.
  if (new_free_x < bin.width) {
    t_box new_free;
    new_free.x = new_free_x;
    new_free.y = old_y;
    new_free.height = old_height;
    if (!is_free_redundant(free_list, new_free)) {
      free_list.insert(new_free);
    }
  }
}


//! Checks to see if the free box is completely covered by another freebox.
bool is_free_redundant (t_free_list& free_list, const t_box& new_free) {
  for (t_free_it it = free_list.begin(); it != free_list.end(); ++it) {
    if (it->x <= new_free.x && it->y <= new_free.y) {
      if (it->top() >= new_free.top()) {
	return true;
      }
    }
  }
  return false;
}


//! Sets the free box's height to a new value or deletes it if the height becomes 0.
void set_free_height (t_free_it free_it, t_free_list& free_list, int height) {
  free_list.erase(free_it);
  if (height <= 0) 
    return;

  t_box free_copy = *free_it;
  free_copy.height = height;
  free_list.insert(free_copy);
}


//! Sets the free box's y to a new value or deletes it if the height becomes 0.
void set_free_y (t_free_it free_it, t_free_list& free_list, int new_y) {
  free_list.erase(free_it);
  int new_height = free_it->height - (new_y - free_it->y);
  if (new_height <= 0) 
    return;

  t_box free_copy = *free_it;
  free_copy.height = new_height;
  free_copy.y = new_y;
  free_list.insert(free_copy);
}


/*******************************************************************************
 * Solver runner
 ******************************************************************************/

/*!
  Properly orders the blocks before running the solution on our dataset.
  It also prints out the results.
*/
void run_packer (t_box_list& list) {

  // Algo requires that every box be taller then they are long.
  for (t_box_it it = list.begin(); it != list.end(); ++it) {
    if (it->height < it->width) {
      std::swap(it->height, it->width);
    }
  }

  // Execute the algo.
  t_box bin = pack_boxes (list);
  print_boxes(list, bin);
  std::cout << bin.area() << std::endl;
}


/*******************************************************************************
 * Tests
 ******************************************************************************/

/*!
  We want our blocks at be at least 3 big so that we can properly see them
  in the pretty pictures.
*/
void run_tests () {

  // Sanity checks of the algorithm.

  {
    t_box_list list;
    list.push_back(t_box(16,16));
    list.push_back(t_box(8,8));
    list.push_back(t_box(4,8));
    list.push_back(t_box(4,4));
    list.push_back(t_box(4,4));
    run_packer(list);
  }

  {
    t_box_list list;
    list.push_back(t_box(16,16));
    list.push_back(t_box(4,12));
    list.push_back(t_box(8,8));
    list.push_back(t_box(4,8));
    list.push_back(t_box(4,4));
    run_packer(list);
  }

  {
    t_box_list list;
    list.push_back(t_box(4,10));
    list.push_back(t_box(4,6));
    list.push_back(t_box(4,6));
    list.push_back(t_box(4,6));
    run_packer(list);
  }
 
  // Lots of similar size boxes.
  //   The algo doesn't perform too well on this one.
  {
    srand(0);
    t_box_list list;
    for (int i = 0; i < 100; ++i) {
      int w = rand() % 47 + 3;
      int h = rand() % 47 + 3;
      list.push_back(t_box(w,h));
    }
    run_packer(list);
  }

  // Few big boxes with lots of small boxes.
  //   In my biased opinion, the algo performs very well on this.
  {
    srand(1);
    t_box_list list;
    for (int i = 0; i < 20; ++i) {
      int w = rand() % 97 + 3;
      int h = rand() % 97 + 3;
      list.push_back(t_box(w,h));
    }
    for (int i = 0; i < 80; ++i) {
      int w = rand() % 17 + 3;
      int h = rand() % 17 + 3;
      list.push_back(t_box(w,h));
    }

    run_packer(list);
  }

}


/*******************************************************************************
 * I/O
 ******************************************************************************/

//! Reads some user input.
bool read_boxes (t_box_list& out_list) {

  int box_count = 0;
  std::cin >> box_count;

  for (int i = 0; i < box_count; ++i) {
    t_box new_box;
    scanf("%d %d", &new_box.width, &new_box.height);
    out_list.push_back(new_box);
  }
  return true;
}


typedef std::vector< std::vector<char> > t_2d_array;

bool print_side (t_2d_array& print_bin, int x, int y, char c);


/*!
  The output will look pretty lopsided since the - char is much smaller
  then the | on the output. So if you think that one block could have
  fitted in that one hole, it probably didn't.
*/
void print_boxes (const t_box_list& box_list, const t_box& bin) {
  
  // std::cerr << std::endl << "Bin(" << bin.width << ", " << bin.height << ")" << std::endl;

  t_2d_array print_scr;
  print_scr.resize(bin.width);
  for (int i = 0; i < bin.width; ++i) {
    print_scr[i].insert(print_scr[i].begin(), bin.height, ' ');
  }

  // We first print to a 2d array which we then output to the stream.
  //   Note that we also try to detect any overlapping squares while doing this.
  //   While that doesn't happen anymore it's still a good idea to check the output.
  for (t_box_cit box_it = box_list.begin(); box_it != box_list.end(); ++box_it) {
    const t_box& box = *box_it; 
    //    box.print();

    // Print the height side.
    for (int i = 0; i < box.height; ++i) {
      char c = i == 0 || i == box.height-1 ? '+' : '-';
      if (!print_side(print_scr, box.x, box.y+i, c)) {
	std::cerr << "ERR: ";
	box.print();
      }
      
      if (box.width == 1) continue;

      if (!print_side(print_scr, box.right()-1, box.y + i, c)) {
	std::cerr << "ERR: ";
	box.print();
      }
    }

    // Print the width side.
    for (int i = 1; i < box.width-1; ++i) {
      if (!print_side(print_scr, box.x+i, box.y, '|')) {
	std::cerr << "ERR: ";
	box.print();
      }
      
      if (box.height == 1) continue;

      if (!print_side(print_scr, box.x+i, box.top()-1, '|')) {
	std::cerr << "ERR: ";
	box.print();
      }
    }
  }

  // Output to console.
  for (int i = 0; i < bin.width; ++i) {
    for (int j = 0; j < bin.height; ++j) {
      std::cerr << print_scr[i][j];
    }
    std::cerr << std::endl;
  }

}


//! Checks for conflicts and dumps a char in the array.
bool print_side (t_2d_array& print_bin, int x, int y, char c) {
  bool err = print_bin[x][y] != ' ';
  print_bin[x][y] = err ? '*' : c; 
  return !err;
}
