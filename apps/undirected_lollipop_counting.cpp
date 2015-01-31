// class templates
#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>* graph;
    long num_triangles;
    MutableGraph *inputGraph;
    size_t num_threads;
    string layout;

    application(Parser input_data){
      num_triangles = 0;
      inputGraph = input_data.input_graph; 
      num_threads = input_data.num_threads;
      layout = input_data.layout;
    }

#ifdef ATTRIBUTES
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;//attribute > 500;
    }
    inline bool myEdgeSelection(uint32_t src, uint32_t dst, uint32_t attribute){
      (void) attribute;
      return attribute == 2012 && src < dst;
    }
#else
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) node; (void) nbr; (void) attribute;
      return true;
    }
    #endif

    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);

      graph = SparseMatrix<T,R>::from_symmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
    }

    inline void queryOver(){
      //graph->print_data("graph.txt");

      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      ParallelBuffer<uint32_t> *y_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->max_nbrhood_size * 4);
      ParallelBuffer<uint32_t> *z_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->max_nbrhood_size * 4);
      ParallelBuffer<uint8_t> *r_buffers = new ParallelBuffer<uint8_t>(num_threads,graph->max_nbrhood_size*sizeof(uint32_t) * 4);

      const size_t matrix_size = graph->matrix_size;
      size_t *t_count = new size_t[num_threads * PADDING];

      y_buffers->allocate(0);
      z_buffers->allocate(0);
      r_buffers->allocate(0);
      uint32_t* y_buffer = y_buffers->data[0];
      uint32_t* z_buffer = z_buffers->data[0];
/*      std::cout << "SET 3" << << std::endl;
      dbg1.foreach([&](uint32_t x) {
          std::cout << x << std::endl;
      });

      std::cout << "SET 552" << std::endl;
      dbg2.foreach([&](uint32_t x) {
          std::cout << x << std::endl;
      });
      Set<R> rs(r_buffers->data[0]);
      Set<R> dbg1 = this->graph->get_decoded_row(1, y_buffer);
      Set<R> dbg2 = this->graph->get_decoded_row(552, z_buffer);
      ops::set_intersect(&rs, &dbg2, &dbg1);

      std::cout << "RESULT " << dbg1.type << " " << dbg2.type << std::endl;
      rs.foreach([&](uint32_t x) {
          std::cout << x << std::endl;
      });

      //dbg1 = this->graph->get_decoded_row(1, y_buffer);
      dbg2 = this->graph->get_decoded_row(553, z_buffer);
      ops::set_intersect(&rs, &dbg2, &dbg1);

      std::cout << "RESULT " << dbg1.type << " " << dbg2.type << std::endl;
      rs.foreach([&](uint32_t x) {
          std::cout << x << std::endl;
      });
      //dbg1 = this->graph->get_decoded_row(1, y_buffer);
      dbg2 = this->graph->get_decoded_row(554, z_buffer);
      ops::set_intersect(&rs, &dbg2, &dbg1);

      std::cout << "RESULT " << dbg1.type << " " << dbg2.type << std::endl;
      rs.foreach([&](uint32_t x) {
          std::cout << x << std::endl;
      });*/

      double start_time = common::startClock();
      common::par_for_range(num_threads, 0, matrix_size, 100,
        [&](size_t tid){
          y_buffers->allocate(tid);
          z_buffers->allocate(tid);
          r_buffers->allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [&](size_t tid, size_t x) {
           Set<R> rs(r_buffers->data[tid]);
           long t_num_lollipops = 0;
           uint32_t* y_buffer = y_buffers->data[tid];
           uint32_t* z_buffer = z_buffers->data[tid];

           Set<R> ys = this->graph->get_decoded_row(x, y_buffer);
           ys.foreach([&](uint32_t y) {
             Set<R> zs = this->graph->get_decoded_row(y, z_buffer);
             //std::cout << x << " " << " " << y << "---" << zs.type << " " << ys.type << std::endl;
             ops::set_intersect(&rs, &ys, &zs);
             //std::cout << rs.number_of_bytes << std::endl;

             rs.foreach([&](uint32_t r) {
               //std::cout << r << std::endl;
               if(y < r) {
                 ys.foreach([&](uint32_t w) {
                   if(w != y && w != r) {
                     t_num_lollipops++;
                   }
                 });
               }
             });
           });

           t_count[tid*PADDING] += t_num_lollipops;
        },
        ////////////////////////////////////////////////////////////
        [&](size_t tid){
          num_triangles += t_count[tid*PADDING];
        }
      );

    common::stopClock("Query",start_time);
    server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
    pcm_print_uncore_power_state(before_uncstate, after_uncstate);
    system_counter_state_t after_sstate = pcm_get_counter_state();
    pcm_print_counter_stats(before_sstate, after_sstate);
  }

  inline void run(){
    double start_time = common::startClock();
    produceSubgraph();
    common::stopClock("Selections",start_time);

    //graph->print_data("graph.txt");

    if(pcm_init() < 0)
       return;

    start_time = common::startClock();
    queryOver();
    common::stopClock("UNDIRECTED TRIANGLE COUNTING",start_time);

    cout << "Count: " << num_triangles << endl << endl;

    common::dump_stats();

    
    pcm_cleanup();
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  Parser input_data = input_parser::parse(argc,argv,"undirected_triangle_counting");

  if(input_data.layout.compare("uint") == 0){
    application<uinteger,uinteger> myapp(input_data);
    myapp.run();
  } else if(input_data.layout.compare("bs") == 0){
    application<bitset,bitset> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("pshort") == 0){
    application<pshort,pshort> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("hybrid") == 0){
    application<hybrid,hybrid> myapp(input_data);
    myapp.run();  
  } 
  #if COMPRESSION == 1
  else if(input_data.layout.compare("v") == 0){
    application<variant,uinteger> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("bp") == 0){
    application<bitpacked,uinteger> myapp(input_data);
    myapp.run();
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }
  return 0;
}
