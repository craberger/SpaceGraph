// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"

using namespace pcm_helper;

namespace application{
  AOA_Matrix *graph;
  long num_triangles = 0;

  inline bool myNodeSelection(uint32_t node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(uint32_t node, uint32_t nbr){
    return nbr < node;
  }
  struct thread_data{
    size_t thread_id;
    uint8_t *buffer;
    uint32_t *decoded_src;

    thread_data(size_t buffer_lengths, const size_t thread_id_in){
      thread_id = thread_id_in;
      decoded_src = new uint32_t[buffer_lengths];
      buffer = new uint8_t[buffer_lengths*sizeof(int)]; //should not be used
    }

    inline long edgeApply(uint32_t src, uint32_t dst){
      long count = graph->row_intersect(buffer,src,dst,decoded_src);
      return count;
    }
  };
  thread_data **t_data_pointers;

  inline void allocBuffers(size_t num_threads){
    t_data_pointers = new thread_data*[num_threads];
    for(size_t k= 0; k < num_threads; k++){
      t_data_pointers[k] = new thread_data(graph->max_nbrhood_size,k);
    }
  }
  inline void queryOver(size_t num_threads){
    auto row_function = std::bind(&AOA_Matrix::sum_over_columns_in_row<long>, graph, _1, _2, _3);

      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      size_t matrix_size = graph->matrix_size;

      thread* threads = new thread[num_threads];
      double* thread_times = new double[num_threads];
      std::atomic<long> reducer;
      reducer = 0;
      const size_t block_size = 1500; //matrix_size / num_threads;
      std::atomic<size_t> next_work;
      next_work = 0;

      if(num_threads > 1){
        for(size_t k = 0; k < num_threads; k++){
          auto edge_function = std::bind(&thread_data::edgeApply,t_data_pointers[k],_1,_2);
          threads[k] = thread([k, &matrix_size, &next_work, &reducer, &t_data_pointers, edge_function, &row_function, &thread_times](void) -> void {
            size_t local_block_size = block_size;
            long t_local_reducer = 0;
            double t_begin = omp_get_wtime();
            while(true) {
              size_t work_start = next_work.fetch_add(local_block_size, std::memory_order_relaxed);
              if(work_start > matrix_size)
                break;

              size_t work_end = min(work_start + local_block_size, matrix_size);
              local_block_size = 10 + (work_start / matrix_size) * block_size;
              for(size_t j = work_start; j < work_end; j++) {
                t_local_reducer += (row_function)(j,t_data_pointers[k]->decoded_src,edge_function);
              }
            }
            reducer += t_local_reducer;
            double t_end = omp_get_wtime();
            thread_times[k] = t_end - t_begin;
           });
        }

        //cleanup
        for(size_t k = 0; k < num_threads; k++) {
          threads[k].join();
        }
      } else{
        auto edge_function = std::bind(&thread_data::edgeApply,t_data_pointers[0],_1,_2);
        long t_local_reducer = 0;
        double t_begin = omp_get_wtime();
        for(size_t i = 0; i < matrix_size; i++){
          t_local_reducer += (row_function)(i,t_data_pointers[0]->decoded_src,edge_function);
        }
        double t_end = omp_get_wtime();
        thread_times[0] = t_end - t_begin;
        reducer = t_local_reducer;
      }

      for(size_t k = 0; k < num_threads; k++){
          std::cout << "Execution time of thread " << k << ": " << thread_times[k] << std::endl;
      }

    server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
    pcm_print_uncore_power_state(before_uncstate, after_uncstate);
    system_counter_state_t after_sstate = pcm_get_counter_state();
    pcm_print_counter_stats(before_sstate, after_sstate);
    num_triangles = reducer;
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        

  std::string input_layout = argv[3];

  common::type layout;
  if(input_layout.compare("a32") == 0){
    layout = common::ARRAY32;
  } else if(input_layout.compare("bs") == 0){
    layout = common::BITSET;
  } else if(input_layout.compare("a16") == 0){
    layout = common::ARRAY16;
  } else if(input_layout.compare("hybrid") == 0){
    layout = common::HYBRID_PERF;
  } 
  #if COMPRESSION == 1
  else if(input_layout.compare("v") == 0){
    layout = common::VARIANT;
  } else if(input_layout.compare("bp") == 0){
    layout = common::A32BITPACKED;
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  auto node_selection = std::bind(&application::myNodeSelection, _1);
  auto edge_selection = std::bind(&application::myEdgeSelection, _1, _2);

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1]); //filename, # of files
  common::stopClock("Reading File");

  common::startClock();
  inputGraph->reorder_by_degree();
  common::stopClock("Reordering");

  cout << endl;

  common::startClock();
  application::graph = AOA_Matrix::from_symmetric(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,inputGraph->max_nbrhood_size,
    node_selection,edge_selection,NULL,layout)->clone_on_node(0);
  application::graph->print_data("tmp3.txt");
  common::stopClock("selections");

  if(pcm_init() < 0)
     return -1;

  //inputGraph->MutableGraph::~MutableGraph(); 

  //application::graph->print_data("out.txt");
  common::startClock();
  application::allocBuffers(atoi(argv[2]));
  common::stopClock("buffer allocation");

  common::startClock();
  application::queryOver(atoi(argv[2]));
  common::stopClock(input_layout);

  cout << "Count: " << application::num_triangles << endl << endl;
  pcm_cleanup();

  return 0;
}
