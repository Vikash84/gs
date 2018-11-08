/******************************************\
| Graph Splitting Method v2.1 (2018/10/15) |
|                                          |
| Copyright (c) 2015-2018 Motomu Matsui    |
|     Distributed under the GNU GPL        |
|                                          |
|     Matsui M and Iwasaki W (2018)        |
|     Systematic Biology, xx:xx-xx.        |
|                                          |
|     http://gs.bs.s.u-tokyo.ac.jp/        |
\******************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <iomanip>

using namespace std;

// Parsing multiple fasta file
int readFASTA(ifstream& ifs, ofstream& ofs1, ofstream& ofs2, int& row){

  int info = 0;

  //Input & Output
  int id = 1;
  int f1 = 0;
  int f2 = 0;

  string line;
  while(getline(ifs, line)){
    if(line[0] == '>'){
      if(f1==1){
	info = 1; //empty entry
	break;
      }
      else{
	auto name = line.substr(1);
	ofs1 << id << "\t" << name << "\n";
	ofs2 << '>' << id << "\n";
	
	id ++;
	f1 = 1;
	f2 = 1;
      }
    }
    else{
      if(f2==0){
	info = 2; //empty entry (1st entry lacked ">")	
	break;
      }
      else{
	ofs2 << line << "\n";
	f1 = 0;
      }
    }
  }

  // # of sequence (= row size of sequence similarity matrix)
  row = id - 1;

  ofs1.close();
  ofs2.close();

  if(f1==1){
    info = 3; // empty entry (last entry lacked a sequence data)
  }
  else if(info==0 && row < 2){
    info = 4; // more than two sequences are required
  }

  return info;
}

// Parsing mmseqs result file
int bl2mat(ifstream& ifs, double* (&W), int const& size){

  //File I/O
  double* S = new double[size*size](); // Bit score matrix
          W = new double[size*size](); // Sequence similarity matrix

  //Reading lines
  int x = 1; // Sequence IDs (query)
  int y = 1; // Sequence IDs (target)
  double b = 0; // Bit scores calculated by MMseqs
  string line, chr;
  while(getline(ifs, line)){
    //Split lines
    istringstream stream(line);

    int pos = 0;
    while(getline(stream, chr, '\t')){
      if(pos == 0){
	x = stoi(chr) -1;
      } 
      else if(pos == 1){
	y = stoi(chr) -1;
      }
      else if(pos == 11){
	b = stod(chr);
      }
      pos ++;
    }

    auto p = x*size+y;
    auto s = S[p];
    S[p] = s>b? s:b;
  }  

  int same_sequence = 0;

  for(int x = 0; x < size; x ++){
    for(int y = x+1; y < size; y ++){
      auto xy = S[x*size+y];
      auto yx = S[y*size+x];      
      auto xx = S[x*size+x];
      auto yy = S[y*size+y];
      
      auto comp = xy>yx?  xy:yx;
      auto self = xx>yy?  xx:yy;
      auto sss  = self>0? comp/self:0;

      W[x*size+y] = sss;
      W[y*size+x] = sss;
      
      if(sss==1){
	same_sequence++;
      }
    }
    W[x*size+x] = 1;
  }

  return same_sequence;
}

// Parsing result file generated by GS method
void sc2nwk(int* const& W, string& newick, int const& size){

  //Input & Output
  int* tr     = new int[size+1](); 
  int* change = new int[size*3]();

  //Reading lines
  for(int r=0; r<size; r++){
    int cur = 1;
    int old = 1;
    for(int c=0; c<size; c++){
      cur = W[r*size+c];
      if(cur != old){
	change[c*3]   = old; // previous membership
	change[c*3+1] = cur; // current membership
	change[c*3+2] ++;    // # of cluster member
      }
      old = cur;
    }
    tr[W[(r+1)*size-1]] = r+1;
  }

  int* nwk = new int[size+1](); for(int p=0; p<size; p++){nwk[p] = 1;}
  int* bra = new int[2*(size+1)](); // # of brachets
  int* pos = new int[2*(size+1)](); // left and right boundary of each cluster
  int* com = new int[size+1]();     // position of commas

  bra[0]        = 1;
  bra[size*2+1] = 1;
  pos[2]        = 0;
  pos[3]        = size;

  for(int p=1; p<size; p++){
    auto old = change[p*3];
    auto cur = change[p*3+1];
    auto num = change[p*3+2];

    auto ps  = pos[old*2];
    auto ls  = pos[old*2+1];
    
    pos[old*2]   += num; // 1 1 1 1 1 -> (2 2),(1 1 1) ... left boundary of cluster 1 = 2
    pos[old*2+1] -= num; // 1 1 1 1 1 -> (2 2),(1 1 1) ... # of cluster 1 = 3
    pos[cur*2]   = ps;   // 1 1 1 1 1 -> (2 2),(1 1 1) ... left boundary of cluster 2 = 0
    pos[cur*2+1] = num;  // 1 1 1 1 1 -> (2 2),(1 1 1) ... # of cluster 2 = 2
    com[ps+num]  = 1;    // 1 1 1 1 1 -> (2 2),(1 1 1) ... position of comma = 2
    
    for(int n=0; n<num; n++){
      nwk[ps+n] = cur;   // 1 1 1 1 1 -> (2 2),(1 1 1) ... replacing 1 -> 2
    }
    
    bra[ps*2]         ++; // (2 2
    bra[(ps+num)*2+1] ++; //     )
    bra[(ps+num)*2]   ++; //      (1 1 1
    bra[(ps+ ls)*2+1] ++; //            )
  }

  newick = "";
  for(int b=0; b<=size; b++){
    bra[b*2]   --; // (
    bra[b*2+1] --; // )

    string right = (bra[b*2+1]>0)? string(bra[b*2+1], ')') : "";
    string left  = (bra[b*2]  >0)? string(bra[b*2],   '(') : "";
    string comma = (com[b]    >0)? string(com[b],     ',') : "";
    string node  = (tr[nwk[b]]>0)? to_string(tr[nwk[b]])   : "";

    newick += right+comma+left+node;
  }
  newick += ";";

  //Free
  delete[] tr;
  delete[] change;
  delete[] nwk;
  delete[] bra;
  delete[] pos;
  delete[] com;
}

void addEP(string const& newick, string& newick_EP, unordered_map<string, double>& ep, int const& ep_num, int const& size){

  newick_EP = "";
  int N = newick.size();
  int B = 0; //brachet
  int f = 1;
  string id = "";

  unordered_map<int, vector<int>> clade;
  stringstream ss;
  stringstream ss_EP;

  for(int n=0; n<N; n++){
    auto p = newick[n];
    ss_EP << p;

    if(p == '('){
      B++;
    }
    else if(p == ')'){

      if(id.size() > 0){
	for(int depth=1; depth<=B; depth++){
	  clade[depth].push_back(stoi(id));
	}
	id = "";
      }

      ss.str("");
      sort(clade[B].begin(), clade[B].end());
      for(int n : clade[B]){
	ss << n << "|";	
      }

      string const C = ss.str();

      if(B>2){
	ss_EP << fixed << setprecision(2) << ep[C]/ep_num;
      }
      else if(B==2 && f==1){
	int count  = 0;
	size_t pos = 0;
	while((pos = C.find("|", pos)) != string::npos){
	  count ++;
	  pos++;
	}

	if(count < size-1){
	  ss_EP << fixed << setprecision(2) << ep[C]/ep_num;
	}

	f=0;
      }
 
      clade[B] = {};
      B--;
    }
    else if(p == ','){
      if(id.size() > 0){
	for(int depth=1; depth<=B; depth++){
	  clade[depth].push_back(stoi(id));
	}
	id = "";
      }
    }
    else{
      id += {p};
    }
  }

  newick_EP = ss_EP.str();
}

void addLABEL(string const& newick, string& newick_ann, string const& annotation_txt, int const& size){

  //File I/O
  ifstream ifs(annotation_txt);   // Annotation file
  string* L = new string[size](); // LABEL: ID->Annotation

  string line, chr;
  while(getline(ifs, line)){

    int pos = 0;
    int id  = 0;
    string ann;
    //Split lines
    istringstream stream(line);
    while(getline(stream, chr, '\t')){
      if(pos == 0){
        id = stoi(chr)-1;
      } 
      else if(pos == 1){
        ann = string(chr);
	break;
      }
      pos ++;
    }

    L[id] = ann;
  }

  newick_ann = "";
  int N = newick.size();
  int s = 0;
  string id = "";
  stringstream ss_ann;

  for(int n=0; n<N; n++){
    auto p = newick[n];

    if(p == '('){
      ss_ann << p;

      s = 1;
    }
    else if(p == ')'){
      if(id.size() > 0){
	ss_ann << '"' << L[stoi(id)-1] << '"';
	id = "";
      }
      ss_ann << p;
      s = 0;
    }
    else if(p == ','){
      if(id.size() > 0){
	ss_ann << '"' << L[stoi(id)-1] << '"';
	id = "";
      }
      ss_ann << p;
      s = 1;
    }
    else if(s==1){
      id += {p};
    }
    else{
      ss_ann << p;
    }
  }

  newick_ann = ss_ann.str();
}
