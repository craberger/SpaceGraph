#include "MutableGraph.hpp"

struct OrderNeighborhoodByDegree{
  vector< vector<unsigned int>*  > *g;
  OrderNeighborhoodByDegree(vector< vector<unsigned int>*  > *g_in){
    g = g_in;
  }
  bool operator()(unsigned int i, unsigned int j) const { 
    return (g->at(i)->size() > g->at(j)->size()); 
  }
};

struct OrderNeighborhoodByRevDegree{
  vector< vector<unsigned int>*  > *g;
  OrderNeighborhoodByRevDegree(vector< vector<unsigned int>*  > *g_in){
    g = g_in;
  }
  bool operator()(unsigned int i, unsigned int j) const { 
    return (g->at(i)->size() < g->at(j)->size()); 
  }
};

void reassign_ids(vector< vector<unsigned int>* > *neighborhoods,vector< vector<unsigned int>* > *new_neighborhoods,unsigned int *new2old_ids,unsigned int *old2new_ids){
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(new2old_ids[i]);
    for(size_t j = 0; j < hood->size(); ++j) {
      hood->at(j) = old2new_ids[hood->at(j)];
    }
    sort(hood->begin(),hood->end());
    new_neighborhoods->push_back(hood);
  }
}
void MutableGraph::reorder_strong_run(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *tmp_new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    tmp_new2old_ids[i] = i;
  }

  std::sort(&tmp_new2old_ids[0],&tmp_new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  unsigned int *new2old_ids = new unsigned int[num_nodes];
  std::unordered_set<unsigned int> *visited = new unordered_set<unsigned int>();
  size_t num_added = 0;
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(tmp_new2old_ids[i]);
    for(size_t j = 0; j < hood->size(); ++j) {
      if(visited->find(hood->at(j)) == visited->end()){
        new2old_ids[num_added++] = hood->at(j);
        visited->insert(hood->at(j));
      }
    }
  }
  delete[] tmp_new2old_ids;

  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}
void MutableGraph::reorder_random(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::random_shuffle(&new2old_ids[0],&new2old_ids[num_nodes]);
  //std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}
void MutableGraph::reorder_by_degree(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}
void MutableGraph::reorder_by_rev_degree(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByRevDegree(neighborhoods));
  
  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}


/*
File format

Edges must be duplicated.  If you have edge (0,1) you must
also have (1,0) listed.

src0 dst1
dst1 src0
...

*/
void MutableGraph::writeUndirectedToBinary(const string path) {
  ofstream outfile;
  outfile.open(path, ios::binary | ios::out);

  size_t osize = out_neighborhoods->size();
  outfile.write((char *)&osize, sizeof(osize)); 
  for(size_t i = 0; i < out_neighborhoods->size(); ++i){
    vector<unsigned int> *row = out_neighborhoods->at(i);
    size_t rsize = row->size();
    outfile.write((char *)&rsize, sizeof(rsize)); 
    outfile.write((char *)row->data(),sizeof(unsigned int)*rsize);
  }
  outfile.close();
}
MutableGraph* MutableGraph::undirectedFromBinary(const string path) {
  ifstream infile; 
  infile.open(path, ios::binary | ios::in); 

  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  size_t num_edges = 0;
  size_t num_nodes = 0;
  infile.read((char *)&num_nodes, sizeof(num_nodes)); 
  for(size_t i = 0; i < num_nodes; ++i){
    size_t row_size = 0;
    infile.read((char *)&row_size, sizeof(row_size)); 
    num_edges += row_size;

    vector<unsigned int> *row = new vector<unsigned int>();
    row->reserve(row_size);
    unsigned int *tmp_data = new unsigned int[row_size];
    infile.read((char *)&tmp_data[0], sizeof(unsigned int)*row_size); 
    row->assign(&tmp_data[0],&tmp_data[row_size]);
    neighborhoods->push_back(row);
  }
  infile.close();

  return new MutableGraph(neighborhoods->size(),num_edges,true,extern_ids,neighborhoods,neighborhoods); 
} 
MutableGraph* MutableGraph::undirectedFromEdgeList(const string path) {
  ////////////////////////////////////////////////////////////////////////////////////
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();

  cout << path << endl;
  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize);
  neighborhoods->reserve(lSize/4);
  extern_ids->reserve(lSize/4);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  char *test = strtok(buffer," \t\nA");
  while(test != NULL){
    unsigned int src;
    sscanf(test,"%u",&src);
    test = strtok(NULL," \t\nA");
    
    unsigned int dst;
    sscanf(test,"%u",&dst);
    test = strtok(NULL," \t\nA");

    vector<unsigned int> *src_row;
    if(extern_ids->find(src) == extern_ids->end()){
      extern_ids->insert(make_pair(src,extern_ids->size()));
      src_row = new vector<unsigned int>();
      neighborhoods->push_back(src_row);
    } else{
      src_row = neighborhoods->at(extern_ids->at(src));
    }

    vector<unsigned int> *dst_row;
    if(extern_ids->find(dst) == extern_ids->end()){
      extern_ids->insert(make_pair(dst,extern_ids->size()));
      dst_row = new vector<unsigned int>();
      neighborhoods->push_back(dst_row);
    } else{
      dst_row = neighborhoods->at(extern_ids->at(dst));
    }

    src_row->push_back(extern_ids->at(dst));
    dst_row->push_back(extern_ids->at(src));
  }
  // terminate
  fclose(pFile);
  free(buffer);

  size_t num_edges = 0;
  for(size_t i = 0; i < neighborhoods->size(); i++){
    vector<unsigned int> *row = neighborhoods->at(i);
    std::sort(row->begin(),row->end());
    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
    num_edges += row->size();
  }

  return new MutableGraph(neighborhoods->size(),num_edges,true,extern_ids,neighborhoods,neighborhoods); 
}
/*
File format

Edges must be duplicated.  If you have edge (0,1) you must
also have (1,0) listed.

src0 dst0 dst1 dst2 dst3 ...
src1 dst0 dst1 dst2 dst3 ...
...

*/
/*
MutableGraph* MutableGraph::undirectedFromAdjList(const string path,const int num_files) {
  vector< vector<unsigned int>* >* *graph_in = new vector< vector<unsigned int>* >*[num_files];

  size_t num_edges = 0;
  size_t num_nodes = 0;
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  
  //#pragma omp parallel for default(none) shared(graph_in,path) reduction(+:num_edges) reduction(+:num_nodes)
  for(size_t i=0; i < (size_t) num_files;++i){
    vector< vector<unsigned int>*  > *file_adj = new vector< vector<unsigned int>* >();

    string file_path = path;
    if(num_files!=1) file_path.append(to_string(i));

    ifstream myfile (file_path);
    string line;
    if (myfile.is_open()){
      while ( getline (myfile,line) ){
        vector<unsigned int> *cur = new vector<unsigned int>(); //guess a size
        cur->reserve(line.length());
        istringstream iss(line);
        do{
          string sub;
          iss >> sub;
          if(sub.compare("")){
            cur->push_back(atoi(sub.c_str()));
          }
        } while (iss);
        cur->resize(cur->size());   
        num_edges += cur->size()-1;
        num_nodes++;
        file_adj->push_back(cur);
      }
      graph_in[i] = file_adj;
    }
  }

  //Serial Merge: Could actually do a merge sort if I cared enough.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods->reserve(num_nodes);
  for(size_t i=0; i < (size_t) num_files;++i){
    neighborhoods->insert(neighborhoods->end(),graph_in[i]->begin(),graph_in[i]->end());
    graph_in[i]->erase(graph_in[i]->begin(),graph_in[i]->end());
  }
  delete[] graph_in;

  //Sort the neighborhoods by degree.
  std::sort(neighborhoods->begin(),neighborhoods->end(),AdjComparator());

  //Build hash map
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  build_hash(neighborhoods,extern_ids);

  reassign_ids(neighborhoods,extern_ids);

  return new MutableGraph(num_nodes,num_edges,true,extern_ids,neighborhoods,neighborhoods); 
  //stopClock("Reassigning ids");
}
*/
/*
File format

src0 dst0
src1 dst1
...

*/
/*
MutableGraph* MutableGraph::directedFromEdgeList(const string path,const int num_files) {
  (void) num_files; //one day this will be in parallel

  size_t num_edges = 0;
  size_t num_nodes = 0;
  
  //Place graph into vector of vectors then decide how you want to
  //store the graph.

  vector<pair<unsigned int,unsigned int>> *edges = new vector<pair<unsigned int,unsigned int>>(); //guess a size

  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize);
  edges->reserve(lSize/4);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  char *test = strtok(buffer," \t\n");
  while(test != NULL){
    unsigned int src;
    sscanf(test,"%u",&src);
    test = strtok(NULL," \t\n");

    unsigned int dst;
    sscanf(test,"%u",&dst);
    test = strtok(NULL," \t\n");

    edges->push_back(make_pair(src,dst));
  }
  // terminate
  fclose(pFile);
  free(buffer);

  num_edges = edges->size();
  ////////////////////////////////////////////////////////////////////
  //out edges
  std::sort(edges->begin(),edges->end(),SrcPairComparator()); //sets us up to remove duplicate sources

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *out_neighborhoods = new vector< vector<unsigned int>* >();
  build_out_neighborhoods(out_neighborhoods,edges); //does flat map

  ////////////////////////////////////////////////////////////////////
  //in edges
  std::sort(edges->begin(),edges->end(),DstPairComparator()); //sets us up to remove duplicate destinations

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *in_neighborhoods = new vector< vector<unsigned int>* >();
  build_in_neighborhoods(in_neighborhoods,edges); //does flat map

  //order by degree
  std::sort(out_neighborhoods->begin(),out_neighborhoods->end(),AdjComparator());

  //Build hash map
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  build_hash(in_neighborhoods,extern_ids); //send this in first to assign it's ID's first
  build_hash(out_neighborhoods,extern_ids);

  //reassign ID's
  reassign_ids(out_neighborhoods,extern_ids);
  reassign_ids(in_neighborhoods,extern_ids);

  std::sort(in_neighborhoods->begin(),in_neighborhoods->end(),NeighborhoodComparator()); //so that we can flatten easily
  std::sort(out_neighborhoods->begin(),out_neighborhoods->end(),NeighborhoodComparator()); //so that we can flatten easily

  num_nodes = extern_ids->size();
  
  delete edges;

  return new MutableGraph(num_nodes,num_edges,false,extern_ids,out_neighborhoods,in_neighborhoods); 
}
*/