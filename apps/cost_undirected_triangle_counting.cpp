#define WRITE_VECTOR 0

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
      (void) attribute;
      return nbr < node;
      //return true;
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

      ParallelBuffer<uint8_t> *src_buffers_ps = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *src_buffers_bs = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));

      ParallelBuffer<uint8_t> *dst_buffers_ps = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *dst_buffers_bs = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));

      ParallelBuffer<uint8_t> *buffers = new ParallelBuffer<uint8_t>(num_threads,graph->max_nbrhood_size*10*sizeof(uint64_t));

      double total_min = 0.0;

      size_t num_uint = 0;
      size_t num_pshort = 0;
      size_t num_bs = 0;
      size_t num_uint_ps = 0;
      size_t num_uint_bs = 0;
      size_t num_ps_bs = 0;

      struct set_stats {
        size_t num_uint;
        size_t num_pshort;
        size_t num_bitset;
        size_t num_uint_ps;
        size_t num_uint_bs;
        size_t num_ps_bs;
        size_t card;
        int64_t min_val;
        int64_t max_val;
        size_t num_hybrid_uint_uint;
        size_t num_hybrid_pshort_pshort;
        size_t num_hybrid_bitset_bitset;
        size_t num_hybrid_uint_pshort;
        size_t num_hybrid_pshort_bitset;
        size_t num_hybrid_uint_bitset;
        double t_time;
        common::type my_type;
      };

      vector<set_stats> stats(graph->matrix_size);
      for(size_t i = 0; i < graph->matrix_size; i++) {
        stats[i].num_uint = 0;
        stats[i].num_pshort = 0;
        stats[i].num_bitset = 0;
        stats[i].card = 0;
        stats[i].min_val = -1;
        stats[i].max_val = -1;
      }

      const size_t matrix_size = graph->matrix_size;
      size_t *t_count = new size_t[num_threads * PADDING];
      common::par_for_range(num_threads, 0, matrix_size, 100,
        [this,src_buffers_ps,src_buffers_bs,dst_buffers_ps,dst_buffers_bs,buffers,t_count](size_t tid){
          src_buffers_ps->allocate(tid);
          src_buffers_bs->allocate(tid);
          dst_buffers_ps->allocate(tid);
          dst_buffers_bs->allocate(tid);
          buffers->allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [&](size_t tid, size_t i) {
           long t_num_triangles = 0;
           uint8_t *src_buffer_ps = src_buffers_ps->data[tid];
           uint8_t *src_buffer_bs = src_buffers_bs->data[tid];

           uint8_t *dst_buffer_ps = dst_buffers_ps->data[tid];
           uint8_t *dst_buffer_bs = dst_buffers_bs->data[tid];

           Set<R> AA = this->graph->get_row(i);

           Set<uinteger> A_uint = AA;
           Set<pshort> A_ps = Set<pshort>::from_array(src_buffer_ps,(uint32_t*)AA.data,AA.cardinality);
           Set<bitset> A_bs = Set<bitset>::from_array(src_buffer_bs,(uint32_t*)AA.data,AA.cardinality);
           stats[i].card = AA.cardinality;

           Set<R> C(buffers->data[tid]);

           AA.foreach([&] (uint32_t j){
            stats[i].min_val = (stats[i].min_val == -1) ? j : min(stats[i].min_val, (int64_t) j);
            stats[i].max_val = (stats[i].max_val == -1) ? j : max(stats[i].max_val, (int64_t) j);

            Set<R> BB = this->graph->get_row(j);

            Set<uinteger> B_uint = BB;
            Set<pshort> B_ps = Set<pshort>::from_array(dst_buffer_ps,(uint32_t*)BB.data,BB.cardinality);
            Set<bitset> B_bs = Set<bitset>::from_array(dst_buffer_bs,(uint32_t*)BB.data,BB.cardinality);

            size_t tmp_count = 0;

            double start_time_1 = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_uint,&B_uint)->cardinality;
            start_time_1 = common::stopClock(start_time_1);

            double start_time_2 = common::startClock();
            tmp_count = ops::set_intersect((Set<pshort>*)&C,&A_ps,&B_ps)->cardinality;
            start_time_2 = common::stopClock(start_time_2);

            double start_time_3 = common::startClock();
            tmp_count = ops::set_intersect((Set<bitset>*)&C,&A_bs,&B_bs)->cardinality;
            start_time_3 = common::stopClock(start_time_3);

            Set<pshort> A_ps_in = (AA.cardinality > BB.cardinality) ? A_ps:B_ps;
            Set<uinteger> B_uint_in = (AA.cardinality > BB.cardinality) ? B_uint:A_uint;

            double start_time_4 = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_ps_in,&B_uint_in)->cardinality;
            start_time_4 = common::stopClock(start_time_4);

            A_ps_in = (AA.cardinality < BB.cardinality) ? A_ps:B_ps;
            Set<bitset> B_bs_in = (AA.cardinality < BB.cardinality) ? B_bs:A_bs;

            double start_time_5 = common::startClock();    
            tmp_count = ops::set_intersect((Set<pshort>*)&C,&A_ps_in,&B_bs_in)->cardinality;
            start_time_5 = common::stopClock(start_time_5);      

            Set<bitset> A_bs_in = (AA.cardinality < BB.cardinality) ? B_bs:A_bs;
            B_uint_in = (AA.cardinality < BB.cardinality) ? A_uint:B_uint;  

            double start_time_6 = common::startClock();    
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_bs_in,&B_uint_in)->cardinality;
            start_time_6 = common::stopClock(start_time_6);

            double min_time = 0.0;

            if(start_time_1 <= start_time_1 &&
              start_time_1 <= start_time_2 &&  
              start_time_1 <= start_time_3 && 
              start_time_1 <= start_time_4 && 
              start_time_1 <= start_time_5 && 
              start_time_1 <= start_time_6){
              num_uint++;
              total_min += start_time_1;
              min_time = start_time_1;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_uint++;
              stats[j].num_uint++;
            } else if(start_time_2 <= start_time_1 &&
              start_time_2 <= start_time_2 &&  
              start_time_2 <= start_time_3 && 
              start_time_2 <= start_time_4 && 
              start_time_2 <= start_time_5 && 
              start_time_2 <= start_time_6){
              num_pshort++;
              min_time = start_time_2;
              total_min += start_time_2;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_pshort++;
              stats[j].num_pshort++;
            } else if(start_time_3 <= start_time_1 &&
              start_time_3 <= start_time_2 &&  
              start_time_3 <= start_time_3 && 
              start_time_3 <= start_time_4 && 
              start_time_3 <= start_time_5 && 
              start_time_3 <= start_time_6){
              num_bs++;
              min_time = start_time_3;
              total_min += start_time_3;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_bitset++;
              stats[j].num_bitset++;
            } else if(start_time_4 <= start_time_1 &&
              start_time_4 <= start_time_2 &&  
              start_time_4 <= start_time_3 && 
              start_time_4 <= start_time_4 && 
              start_time_4 <= start_time_5 && 
              start_time_4 <= start_time_6){
              num_uint_ps++;
              min_time = start_time_4;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              total_min += start_time_4;
              stats[i].num_uint_ps++;
              stats[j].num_uint_ps++;
              /*
              if(AA.cardinality < BB.cardinality) {
                stats[i].num_uint++;
                stats[j].num_pshort++;
              } else {
                stats[j].num_uint++;
                stats[i].num_pshort++;
              }
              */
            } else if(start_time_5 <= start_time_1 &&
              start_time_5 <= start_time_2 &&  
              start_time_5 <= start_time_3 && 
              start_time_5 <= start_time_4 && 
              start_time_5 <= start_time_5 && 
              start_time_5 <= start_time_6){
              num_ps_bs++;
              min_time = start_time_5;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              total_min += start_time_5;
              stats[i].num_ps_bs++;
              stats[j].num_ps_bs++;
              /*
              if(AA.cardinality < BB.cardinality) {
                stats[i].num_pshort++;
                stats[j].num_bitset++;
              } else {
                stats[j].num_pshort++;
                stats[i].num_bitset++;
              }*/
            } else {
              num_uint_bs++;
              min_time = start_time_6;
              total_min += start_time_6;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_uint_bs++;
              stats[j].num_uint_bs++;
              /*
              if(AA.cardinality < BB.cardinality) {
                stats[i].num_uint++;
                stats[j].num_bitset++;
              } else {
                stats[j].num_uint++;
                stats[i].num_bitset++;
              }
              */
            }

            double min_ratio = 2.0;
            double hybrid_intersection_time = 0.0;
            common::type a_type = hybrid::get_type((uint32_t*)AA.data,AA.cardinality);
            common::type b_type = hybrid::get_type((uint32_t*)BB.data,BB.cardinality);
            if(min_time > 0.0){
              stats[i].my_type = a_type;
              stats[j].my_type =b_type;
              if(a_type == common::UINTEGER && b_type == common::UINTEGER){
                hybrid_intersection_time = start_time_1;
                if((hybrid_intersection_time/min_time) > min_ratio){
                  stats[i].num_hybrid_uint_uint++;
                  stats[j].num_hybrid_uint_uint++;
                }
              } else if(a_type == common::PSHORT && b_type == common::PSHORT){
                hybrid_intersection_time = start_time_2;
                if((hybrid_intersection_time/min_time) > min_ratio){
                  stats[i].num_hybrid_pshort_pshort++;
                  stats[j].num_hybrid_pshort_pshort++;
                }
              } else if(a_type == common::BITSET && b_type == common::BITSET){
                hybrid_intersection_time = start_time_3;
                if((hybrid_intersection_time/min_time) > min_ratio){
                  stats[i].num_hybrid_bitset_bitset++;
                  stats[j].num_hybrid_bitset_bitset++;
                }
              } else if( (a_type == common::UINTEGER && b_type == common::PSHORT) ||
                (a_type == common::PSHORT && b_type == common::UINTEGER)){
                hybrid_intersection_time = start_time_4;
                if((hybrid_intersection_time/min_time) > min_ratio){
                    stats[i].num_hybrid_uint_pshort++;
                    stats[j].num_hybrid_uint_pshort++;
                }
              }  else if( (a_type == common::BITSET && b_type == common::PSHORT) ||
                (a_type == common::PSHORT && b_type == common::BITSET)){
                hybrid_intersection_time = start_time_5;
                if((hybrid_intersection_time/min_time) > min_ratio){
                    stats[i].num_hybrid_pshort_bitset++;
                    stats[j].num_hybrid_pshort_bitset++;
                }
              }  else if((a_type == common::UINTEGER && b_type == common::BITSET) ||
                (a_type == common::BITSET && b_type == common::UINTEGER)){
                hybrid_intersection_time = start_time_6;
                if((hybrid_intersection_time/min_time) > min_ratio){
                    stats[i].num_hybrid_uint_bitset++;
                    stats[j].num_hybrid_uint_bitset++;
                }
              } 
            }
    
            t_num_triangles += tmp_count;
           });

           t_count[tid*PADDING] += t_num_triangles;
        },
        ////////////////////////////////////////////////////////////
        [this,t_count](size_t tid){
          num_triangles += t_count[tid*PADDING];
        }
      );

    cout << "Best cost time: " << total_min << endl;
    cout << "Uint: " << num_uint << endl;
    cout << "Pshort: " << num_pshort << endl;
    cout << "Bs: " << num_bs << endl;
    cout << "Uint/pshort: " << num_uint_ps << endl;
    cout << "PS/BS: " << num_ps_bs << endl;
    cout << "UINT/BS: " << num_uint_bs << endl;

    ofstream stats_file;
    stats_file.open("stats.csv");
    stats_file << "id,card,range,uint/uint,pshort/pshort,bitset/bitset,u/ps,ps/bs,u/bs,,eh uint/uint,eh pshort/pshort,eh bitset/bitset,eh u/ps,eh ps/bs,eh u/bs,percent time,ehtype" << std::endl;
    for(size_t i = 0; i < graph->matrix_size; i++) {
      stats_file << i << ",";
      stats_file << stats[i].card << ",";
      stats_file << (stats[i].max_val - stats[i].min_val) << ",";
      stats_file << stats[i].num_uint << ",";
      stats_file << stats[i].num_pshort << ",";
      stats_file << stats[i].num_bitset << ",";
      stats_file << stats[i].num_uint_ps << ",";
      stats_file << stats[i].num_ps_bs << ",";
      stats_file << stats[i].num_uint_bs << ",,";
      stats_file << stats[i].num_hybrid_uint_uint << ",";
      stats_file << stats[i].num_hybrid_pshort_pshort << ",";
      stats_file << stats[i].num_hybrid_bitset_bitset << ",";
      stats_file << stats[i].num_hybrid_uint_pshort << ",";
      stats_file << stats[i].num_hybrid_pshort_bitset << ",";
      stats_file << stats[i].num_hybrid_uint_bitset << ",";
      stats_file << ((double)stats[i].t_time/total_min) << ",";
      if(stats[i].my_type == common::BITSET)
        stats_file << "bitset" <<std::endl;
      else if(stats[i].my_type == common::PSHORT)
        stats_file << "pshort" <<std::endl;
      else 
        stats_file << "uint" <<std::endl;

    }

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

  application<uinteger,uinteger> myapp(input_data);
  myapp.run();
  return 0;
}
