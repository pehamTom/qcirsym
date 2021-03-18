#include <stdlib.h>
extern "C" {
#include "../saucy/saucy.h"
}

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <math.h>
#include <algorithm>

/**
/ Type definitions
**/
enum VarType: uint8_t {varGate, andGate, orGate, notGate};

struct Gate {
  int32_t fileVarId; //variable id in source file
  std::vector<int32_t> adj; //connected gates
  VarType type;

  Gate(int32_t fileVarId, VarType type): fileVarId(fileVarId), type(type){}
};

typedef std::vector<std::vector<int32_t>> color_map_t;
typedef std::unordered_map<int32_t, int32_t> var_map_t;

struct QCIR_Info {
  uint32_t numVars;
  uint32_t numColors;
  std::vector<Gate> gates;
  color_map_t colorMap;
  var_map_t fileToGraphMap;
  
  
  QCIR_Info():numVars(0), numColors(0){}
};

/*
/ Constants and global variables
*/
static const uint32_t RETURN_SUCCESS = 0;
static const uint32_t RETURN_FAIL = -1;

std::vector<std::vector<std::vector<int>>> orbits;
char* marks;


void printErrAndExit(const std::string& errMsg) {
  std::cerr << errMsg << std::endl;
  exit(RETURN_FAIL);
}

/* used for quicksort */
int integer_compare(const void *a, const void *b)
{
  const int *aa = (const int *) a, *bb = (const int*) b;
  return *aa < *bb ? -1 : *aa == *bb ? 0 : 1;
}

/*utility procedure for storing variable information*/
void storeVariable(QCIR_Info& info, int32_t fileVarId) {
  /*both literals need to appear in graph*/
  Gate var(fileVarId, varGate);
  Gate negVar(-fileVarId, varGate);

  /*literals belonging to same variable are always connected*/
  var.adj.push_back(-fileVarId);
  negVar.adj.push_back(fileVarId);
      
  info.gates.push_back(var);
  info.gates.push_back(negVar);

  /*both literals have same color, so no need to store both*/
  info.colorMap.back().push_back(fileVarId);

  /*store actual variable indices*/
  info.fileToGraphMap[fileVarId] = info.numVars++;
  info.fileToGraphMap[-fileVarId] = info.numVars++;
}

/*utility procedure for storing variables belonging to gate*/
void readGateVar(QCIR_Info& info, std::unordered_map<int32_t, std::vector<int32_t>>& adjToAdd, const int32_t gateId, const size_t gateIndex, const int32_t varId) {

  /*if gate is negated and not a literal we need to create new gate node*/
  auto varIterator = info.fileToGraphMap.find(-varId);
  if(varId < 0 && (varIterator == info.fileToGraphMap.end() || (size_t)varIterator->second >= info.numVars)) {
    Gate not_(varId, notGate);
    Gate &cachedNot = not_;

    /*only create not-gate once*/
    if(info.fileToGraphMap.find(varId) == info.fileToGraphMap.end()) {
      info.fileToGraphMap[varId] = info.gates.size();
      cachedNot.adj.push_back(-varId);
      info.gates.push_back(cachedNot);
      adjToAdd[-varId].push_back(varId);
    }
    info.gates[info.fileToGraphMap[varId]].adj.push_back(gateId);
    info.gates[gateIndex].adj.push_back(varId);
    
  }else {
    info.gates[gateIndex].adj.push_back(varId);
    adjToAdd[varId].push_back(gateId);
  }
}

/*exception handling parsing*/
int32_t parseInt(std::string s, size_t from, size_t to, size_t lineNumber) {
   int32_t i;    
   try {
     i = std::stoi(s.substr(from, to-from));
   } catch(std::invalid_argument& e) {
     printErrAndExit("Not a valid variable at line " + std::to_string(lineNumber));
   }
    return i;
}

/* parse file and graph structure */
QCIR_Info parseQCIRFile(const std::string& fileName) {
  std::string s;
  std::ifstream file(fileName);
  if(! file.is_open()) {
    printErrAndExit("Couldn't open " + fileName);
  }
  getline(file, s);
  if(s.compare(0, 9, "#QCIR-G14") != 0)
    printErrAndExit("Not a valid QCIR File");
  
  size_t lineNumber = 1;
  
  QCIR_Info info;
  
  while(getline(file, s)) {

    size_t commentPos = s.find('#');
    size_t parPos = s.find(')');
    if(commentPos <= parPos) continue; //ignore comments
    
    if(s.compare(0, 6, "forall") != 0 && s.compare(0, 6, "exists") != 0)
      break;
    if(s[6] != '(') {
      file.close();
      printErrAndExit("Not a valid QCIR File. Expected \"(\" at line " + std::to_string(lineNumber));
    }

    /*next variable is between these two positions in string*/
    size_t from = 7;
    size_t to = 0;

    /*create new color partition*/
    info.colorMap.push_back(std::vector<int32_t>());
    
    /*read quantified variables*/
    while((to = s.find(',', from)) != std::string::npos) {
      int32_t fileVarId = parseInt(s, from, to, lineNumber);
      storeVariable(info, fileVarId);
        
      from = to+1;
    }
    
    if((to = s.find(')', from)) == std::string::npos) {
      file.close();
      printErrAndExit("Not a valid QCIR File. Missing \")\" at line " + std::to_string(lineNumber));
    }
    
    /*read last variable in line*/
    int32_t fileVarId = parseInt(s, from, to, lineNumber);
    storeVariable(info, fileVarId);

    /*variables belonging to different quantifier have different color*/
    info.numColors++;
    
    lineNumber++;
    
  }
  
  //read output gate
  if(s.compare(0,6, "output") != 0) {
    file.close();
    printErrAndExit("Not a valid QCIR File. Expected keyword \"output\" at line " + std::to_string(lineNumber));
  }

  /*store back-edges*/
  std::unordered_map<int32_t, std::vector<int32_t>> adjToAdd;
  
  /*read gates*/
  while(getline(file, s)) {
    lineNumber++;
    
    size_t eqIndex = s.find('=');
    if(eqIndex == std::string::npos) {
      file.close();
      printErrAndExit("Not a valid QCIR File. Missing \"=\" at line " + std::to_string(lineNumber));
    }
    
    int32_t gateId = parseInt(s, 0, eqIndex, lineNumber);
    
    std::string gateName = s.substr(eqIndex+1, s.find('(')-eqIndex-1);
    
    gateName.erase(std::remove_if(gateName.begin(), gateName.end(), isspace), gateName.end());
    Gate gate(gateId, orGate);
    size_t gateIndex = info.gates.size();
    if(gateName == "or") {
      info.fileToGraphMap[gateId] = gateIndex;
      info.gates.push_back(gate);
    } else if(gateName == "and") {
      gate.type = andGate;
      info.fileToGraphMap[gateId] = gateIndex;
      info.gates.push_back(gate);
    } else{
      file.close();
      printErrAndExit("Only \"and\" and \"or\" gates are allowed. Line number " + std::to_string(lineNumber));
    }
    
    /*next variable is between these two positions in string*/
    size_t from = s.find('(')+1;
    size_t to = 0;

    std::vector<int32_t> vars;
    
    /*read variables from gate*/
    while((to = s.find(',', from)) != std::string::npos) {
      int32_t varId = parseInt(s, from, to, lineNumber);;
        
      readGateVar(info, adjToAdd, gateId, gateIndex, varId);
      from = to+1;
    }

    /*check if gate doesn't connect to any others*/
    size_t startParPos = s.find('(');
    size_t endParPos = s.find(')');
    if(s.substr(startParPos+1, endParPos-startParPos-1).find_first_not_of(' ') != std::string::npos) {
    /*read last variable in line*/
      int32_t varId = parseInt(s, from, to, lineNumber);
      readGateVar(info, adjToAdd, gateId, gateIndex, varId);
    } 
  }

  /*add backedges*/
  for(auto entry: adjToAdd) {
    int32_t graphVarId = info.fileToGraphMap[entry.first];
    Gate& g = info.gates[graphVarId];
    for(int32_t toAdd: entry.second) {
      g.adj.push_back(toAdd);
    }
  }

  return info;
}

/* executed when saucy finds automorphism during search */
static int onAutomorphismStoreOrbits(int n, const int *gamma, int nsupp, int *support, void *arg) {
  /* unused variables */
  (void) n;
  (void) arg;
  
  /*sort support*/
  qsort(support, nsupp, sizeof(int), integer_compare);
  
  int i, j, k;
  orbits.push_back(std::vector<std::vector<int>>());
  std::vector<std::vector<int>>& orbit = orbits.back();
  for (i = 0; i < nsupp; ++i) {
    k = support[i];
    
    /* Skip elements already seen */
    if (marks[k]) continue;
    
    /* Start an orbit */
    marks[k] = 1;

    orbit.push_back(std::vector<int>());
    std::vector<int>& permutation = orbit.back();
    permutation.push_back(k);
    
    /* Mark and notify elements in this orbit */
    for (j = gamma[k]; j != k; j = gamma[j]) {
      
      marks[j] = 1;
      permutation.push_back(j);
    }
    
  }
  /* Clean up */
  for (i = 0; i < nsupp; ++i) {
    marks[support[i]] = 0;
  }
  return 1;
}

int main(int argc, char** argv) {
  if(argc < 1)
    printErrAndExit("No File given");

  QCIR_Info info = parseQCIRFile(argv[1]);
  

  /* prepare graph structure for saucy */
  std::vector<int32_t> adj;
  std::vector<int32_t> edg;
  for(const Gate& g: info.gates) {
    adj.push_back(edg.size());
    for(const int32_t id: g.adj) {
      edg.push_back(info.fileToGraphMap[id]);
    }
  }

  /*prepare color mapping for saucy */
  std::vector<int> colors(adj.size());

  size_t colorIndex = 0;

  /*store color info for variables*/
  for(size_t color = 0; color < info.colorMap.size(); color++) {
    for(size_t i = 0; i <  info.colorMap[color].size(); i++) {
      /*literals belonging to same variable are consecutive in node list*/
      colors[colorIndex++] = color;
      colors[colorIndex++] = color;
    }
  }

  /*store color info for gates*/
  for(;colorIndex < colors.size(); colorIndex++) {
    switch(info.gates[colorIndex].type) {
    case andGate: {
      colors[colorIndex] = info.numColors;
      break;
    }
    case orGate: {
      colors[colorIndex] = info.numColors + 1;
      break;
    }
    case notGate: {
      colors[colorIndex] = info.numColors + 2;
      break;
    }
    default: {
      break;
    }
    }
  }

  /*allocate resources for saucy*/
  saucy* s = saucy_alloc(info.gates.size());
  saucy_graph g;
  marks = (char*)calloc(info.gates.size(), sizeof(char));

  adj.push_back(edg.size());
  g.adj = adj.data();
  g.edg = edg.data();

  g.n = adj.size()-1;
  g.e = edg.size();

  
  saucy_stats stats; //dummy variable for saucy
  
  /*free saucy resources*/
  saucy_search(s, &g, 0, colors.data(), onAutomorphismStoreOrbits, NULL, &stats);
  saucy_free(s);
  free(marks);

  /*map orbits back to actual variables and print*/
  for(auto orbit: orbits) {
    bool foundOrbit = false;
    for(auto permutation: orbit) {
      std::vector<int32_t> varOrbit; 

      /*filter out gate variables*/
      for(int32_t varId: permutation) {
        if(abs(varId) < info.numVars) {
          varOrbit.push_back(info.gates[varId].fileVarId);
        }
      }

      if(varOrbit.size() == 0) continue;
      foundOrbit = true;
      /*print variables*/      
      std::cout << "(";
      for(size_t i = 0; i < varOrbit.size()-1; i++) {
        std::cout << varOrbit[i] << " ";
      }
      std::cout << varOrbit.back() << ")";
    }
    if(foundOrbit)
      std::cout << std::endl;
  }

  return RETURN_SUCCESS;
}

