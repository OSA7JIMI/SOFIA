// © Copyright IBM Corporation 2022. All Rights Reserved.
// LICENSE: Eclipse Public License - v 2.0, https://opensource.org/licenses/EPL-2.0
// SPDX-License-Identifier: EPL-2.0

#include "Model2MasterLP.hpp"

//#include <iostream>
#include <iomanip>

using namespace std;

Model2MasterLP::Model2MasterLP(int relationId, Data& d, Parameters& params)
  :relationId_(relationId), dat_(d), params_(params)
{
  n_ = dat_.getNumPairsQuery(relationId_);
  r_ = dat_.getNumberRelations();

  numNonZerosPerRow_.resize(n_);
  minPercentCoverage_ = 0.0;
  penaltyOnPairsExtraCoverage_ = 0.0;

  useFrequencyBias_ = params_.getUseFrequencyBias();

  if(useFrequencyBias_) bias_ = dat_.getRelationFrequencyBias()[relationId_];
  else bias_ = 0.0;

  useSoftMargin_ = params_.getUseSoftMargin();
  gamma_ = params_.getMarginValue();
  lambda_ = params_.getSlackPenalty();

  numNegSamples_ = 30;

  if(useSoftMargin_) {
    sampleNegativePairs(relationId);
    m_ = (int)negativePairs_.size();
    if (m_ == 0) {
      useSoftMargin_ = false;
      cout<<"Soft margin set to false"<<endl;
    }
  }

  model_ = IloModel(env_);
  createModelStructure();
  setInitParams();
}

void Model2MasterLP::sampleNegativePairs(int relationId)
{
  negativePairs_.clear();

  if (params_.getReportStatsRelation()) {
    // Sample from ALL other relations
    for(int r=0; r<r_; r++) {
      if(r == relationId) continue;  
      
      // Get all pairs for this relation
      Query& query = dat_.getQuery(r);
      vector<pair<int,int>>& pairs = query.getEntityPairs();
      
      int numPairsToSample = min(numNegSamples_, (int)pairs.size());
      
      for(int i=0; i<numPairsToSample; i++) {
        int idx = rand() % pairs.size();
        negativePairs_.push_back(pairs[idx]);
      }
    }

    cout << "Sampled " << negativePairs_.size() 
      << " negative pairs from " << (r_-1) << " other relations" << endl;
  }

  else {
    Query& query = dat_.getQuery(relationId);
    vector<pair<int,int>>& posPairs = query.getEntityPairs();
    set<pair<int,int>> posSet(posPairs.begin(), posPairs.end());
    
    // Get entities that ACTUALLY appear with this relation
    set<int> validHeads, validTails;
    for(int i=0; i<(int)posPairs.size(); i++) {
      validHeads.insert(posPairs[i].first);
      validTails.insert(posPairs[i].second);
    }
    
    vector<int> headList(validHeads.begin(), validHeads.end());
    vector<int> tailList(validTails.begin(), validTails.end());
    
    int samplesNeeded = numNegSamples_ * (r_ - 1);
    int attempts = 0;
    int maxAttempts = samplesNeeded * 20;
    
    while(negativePairs_.size() < samplesNeeded && attempts < maxAttempts) {
      pair<int,int> posPair = posPairs[rand() % posPairs.size()];
      pair<int,int> negPair;
      
      if(rand() % 2 == 0) {
        // Corrupt head - but choose from VALID heads only
        int newHead = headList[rand() % headList.size()];
        negPair = pair<int,int>(newHead, posPair.second);
      } else {
        // Corrupt tail - but choose from VALID tails only  
        int newTail = tailList[rand() % tailList.size()];
        negPair = pair<int,int>(posPair.first, newTail);
      }
      
      // Only add if it's actually negative
      if(posSet.find(negPair) == posSet.end()) {
        negativePairs_.push_back(negPair);
      }
      
      attempts++;
    }
  }
}

void Model2MasterLP::createModelStructure()
{
  try {
    // initialize variables
    eta_ = IloNumVarArray(env_, n_, 0, 1, ILOFLOAT);
    eta_.setNames("_eta");
    x_ = IloNumVarArray(env_);
    w_ = IloNumVarArray(env_);

  if(useSoftMargin_) {
    // Create slack variables: one per (positive, negative) pair
    xi_ = IloNumVarArray(env_, n_, 0, IloInfinity, ILOFLOAT);
    xi_.setNames("_xi");
    
   }

    // add objective function
    IloNumExpr exp(env_);
    for (IloInt i=0; i<n_; i++)
      exp += eta_[i];

  if(useSoftMargin_) {
    for (int i=0; i<n_; i++){
      exp += lambda_ * xi_[i];
    }

  }

    obj_ = IloObjective(env_, exp, IloObjective::Minimize, "obj");
    model_.add(obj_);
    exp.end();

    // add constraint 7
    con7_ = IloRange(env_, -IloInfinity, dat_.getMaxComplexity(), "Cardinality");
    model_.add(con7_);

    // add constraints 11
    con11_ = IloRangeArray(env_);
    for (IloInt i=0; i<n_; i++) {
      IloNumExpr exp(env_);
      exp += eta_[i];

      stringstream ss;
      ss << "con11." << con11_.getSize();
      string s = ss.str();
      con11_.add(IloRange(env_, 1-bias_, exp, IloInfinity, s.c_str()));
      exp.end();
    }
    model_.add(con11_);

    if(useSoftMargin_) {
    conMargin_ = IloRangeArray(env_);
    
    for(int i=0; i<n_; i++) {
      IloNumExpr exp(env_);
      exp += xi_[i];
      
      stringstream ss;
      ss << "margin_" << i;
      conMargin_.add(IloRange(env_, gamma_-bias_, exp, IloInfinity, ss.str().c_str()));
      exp.end();
    }
    
    model_.add(conMargin_);
    cout << "Added "<< conMargin_.getSize()<< " aggregate margin constraints"<<endl;
  }

    // initialize constraints 6
    con6_ = IloRangeArray(env_);

    cpx_ = IloCplex(model_);

  }
  catch (IloException& e) {
    cerr << "Concert exception caught: " << e << endl;
  }
  catch (...) {
    cerr << "Unknown exception caught" << endl;
  }

}

void Model2MasterLP::setMaxComplexity(int maxComplexity)
{
  con7_.setUB(maxComplexity);
}

bool Model2MasterLP::addCol(Rule& rule, double objPenalty)
{
  vector<int> numpaths; // each entry is either 0 or 1
  dat_.getNumPaths(relationId_, rule, numpaths);
  bool coladded = addCol(rule, numpaths, objPenalty);
  return coladded;
}

bool Model2MasterLP::addCol(Rule& rule, vector<int>& column, double objPenalty)
{
  assert(n_ == (int)column.size());
  int rows_covered = 0;
  for (int i=0; i<n_; i++) {
    if(column[i] > 0) {
      rows_covered += 1;
      numNonZerosPerRow_[i]++;
    }
  }

  if(!isThereEnoughCoverage(rows_covered)) {
    return false;
  }
  else {
    numNonZerosPerCol_.push_back(rows_covered);
  }

  bool coladded = addColToLP(rule, column, objPenalty);
  return coladded;
}

bool Model2MasterLP::addCol(Rule& rule, vector<double>& column, double objPenalty)
{
  assert(n_ == (int)column.size());
  int rows_covered = 0;
  for (int i=0; i<n_; i++) {
    if(column[i] > 0) {
      rows_covered += 1;
      numNonZerosPerRow_[i]++;
    }
  }

  if(!isThereEnoughCoverage(rows_covered)) {
    return false;
  }
  else {
    numNonZerosPerCol_.push_back(rows_covered);
  }

  bool coladded = addColToLP(rule, column, objPenalty);
  return coladded;
}

bool Model2MasterLP::addColToLP(Rule& rule, vector<int>& column, double objPenalty)
{
  double objx = objPenalty*(1+rule.getLengthRule());
  xObjValues_.push_back(objx);
  IloNumColumn colx = obj_(objx);
  IloNumColumn colw = obj_(0);

  for (int i=0; i<n_; i++) {
    if(column[i] > 0) {
      colw += con11_[i](column[i]);
    }
  }

  IloRange newcon;
  {
    stringstream ss;
    ss << "con6." << con6_.getSize();
    string s = ss.str();
    newcon = IloRange(env_, -IloInfinity, 0, s.c_str());
    con6_.add(newcon);
    model_.add(newcon);
    colw += newcon(1);
    colx += newcon(-1);
  }

  colx += con7_(1+rule.getLengthRule());

    vector<int> negColumn;
  if(useSoftMargin_) {
  int negativesFired = 0;
  
  for(int j=0; j<m_; j++) {
    if(dat_.hasPath(rule, negativePairs_[j])) {
      negativesFired++;
    }
  }
  
  double avgNegCov = (double)negativesFired / m_;
  double specificity = 1.0 - avgNegCov;
  ruleSpecificity_.push_back(specificity);
  
  for(int i=0; i<n_; i++) {
    double coef = column[i] - avgNegCov;
    if(coef != 0.0) {
      colw += conMargin_[i](coef);
    }
  }
}

  {
    stringstream ss;
    ss << "x" << x_.getSize();
    string s = ss.str();
    x_.add(IloNumVar(colx, 0, 1, ILOFLOAT, s.c_str()));
  }
  {
    stringstream ss;
    ss << "w" << w_.getSize();
    string s = ss.str();
    w_.add(IloNumVar(colw, 0, 1, ILOFLOAT, s.c_str()));
  }

  //  newcon.end();
  colx.end();
  colw.end();

  return true;
}

bool Model2MasterLP::addColToLP(Rule& rule, vector<double>& column, double objPenalty)
{
  IloNumColumn colx = obj_(objPenalty*(1+rule.getLengthRule()));
  IloNumColumn colw = obj_(0);

  for (int i=0; i<n_; i++) {
    if(column[i] > 0) {
      colw += con11_[i](column[i]);
    }
  }

  IloRange newcon;
  {
    stringstream ss;
    ss << "con6." << con6_.getSize();
    string s = ss.str();
    newcon = IloRange(env_, -IloInfinity, 0, s.c_str());
    con6_.add(newcon);
    model_.add(newcon);
    colw += newcon(1);
    colx += newcon(-1);
  }

  colx += con7_(1+rule.getLengthRule());

  {
    stringstream ss;
    ss << "x" << x_.getSize();
    string s = ss.str();
    x_.add(IloNumVar(colx, 0, 1, ILOFLOAT, s.c_str()));
  }
  {
    stringstream ss;
    ss << "w" << w_.getSize();
    string s = ss.str();
    w_.add(IloNumVar(colw, 0, 1, ILOFLOAT, s.c_str()));
  }

  colx.end();
  colw.end();

  return true;
}

void Model2MasterLP::solveModel(bool writeLpFile)
{
  try {
    if(writeLpFile)
      cpx_.exportModel("model.lp");

    // Optimize the problem and obtain solution.
    if ( !cpx_.solve() ) {
      env_.error() << "Failed to optimize LP" << endl;
      throw(-1);
    }

    IloNumArray vals(env_);
    env_.out() << "Solution status = " << cpx_.getStatus() << endl;
    env_.out() << "Solution value  = " << cpx_.getObjValue() << endl;
#if 0
    cpx_.getValues(vals, x_);
    env_.out() << "Values x       = " << vals << endl;
    cpx_.getValues(vals, w_);
    env_.out() << "Values w       = " << vals << endl;


    env_.out() << "Slacks        = " << vals << endl;
    cpx_.getDuals(vals, con);
    env_.out() << "Duals         = " << vals << endl;
    cpx_.getReducedCosts(vals, x_);
    env_.out() << "Reduced Costs x = " << vals << endl;
    cpx_.getReducedCosts(vals, w_);
    env_.out() << "Reduced Costs w = " << vals << endl;
#endif
  }
  catch (IloException& e) {
    cerr << "Concert exception caught: " << e << endl;
  }
  catch (...) {
    cerr << "Unknown exception caught" << endl;
  }
  
}

void Model2MasterLP::setInitParams()
{
  try {
    int numberThreads = 1;
    cpx_.setParam(IloCplex::Param::Threads, numberThreads);
    env_.out() << "Maximum number of threads in CPLEX: " << numberThreads << endl;
    int lpAlgorithm = 0; // 0 (automatic), 1 (primal) 2 (Dual)
    cpx_.setParam(IloCplex::Param::RootAlgorithm, lpAlgorithm);
    env_.out() << "LP algorithm: " << lpAlgorithm << endl;

  }
  catch (IloException& e) {
    cerr << "Concert exception caught: " << e << endl;
  }
  catch (...) {
    cerr << "Unknown exception caught" << endl;
  }
  
}

void Model2MasterLP::getSolution(vector<double>& x, vector<double>& w)
{
  try {
    IloNumArray vals(env_);

    assert(x.size() == 0);
    cpx_.getValues(vals, x_);
    for (IloInt i=0; i<vals.getSize(); i++)
      x.push_back(vals[i]);

    assert(w.size() == 0);
    cpx_.getValues(vals, w_);
    for (IloInt i=0; i<vals.getSize(); i++)
      w.push_back(vals[i]);

  }
  catch (IloException& e) {
    cerr << "Concert exception caught: " << e << endl;
  }
  catch (...) {
    cerr << "Unknown exception caught" << endl;
  }

}


double Model2MasterLP::getDuals(vector<double>& duals_con11)
{
  double dual_con7;

  try {
    assert(duals_con11.size() == n_);
    for (IloInt i=0; i<n_; i++)
      duals_con11[i] = cpx_.getDual(con11_[i]);

    double dual_con7 = cpx_.getDual(con7_);
    return dual_con7;
  }
  catch (IloException& e) {
    cerr << "Concert exception caught: " << e << endl;
  }
  catch (...) {
    cerr << "Unknown exception caught" << endl;
  }

}

double Model2MasterLP::getReducedCost(Rule& rule, vector<int>& column, vector<double>& duals_con11)
{
  if(column.size()==0) {
    dat_.getNumPaths(relationId_, rule, column);
  }

  double rc = 0.0;
  for (int i=0; i<n_; i++)
    rc -= duals_con11[i] * column[i];
  return rc;
}

void Model2MasterLP::printLPStatistics()
{
  int npairscovered = 0;
  for(int i=0; i<(int)numNonZerosPerRow_.size(); i++) {
    if(numNonZerosPerRow_[i] > 0)
      npairscovered++;
  }

  cout<<"number of pairs in query covered by at least one column: "<<npairscovered<<endl;
  if (params_.getAddPenaltyOnNegativePairs()){

  cout<<"number nonzeros per column: ";
  for(int i=0; i<(int)numNonZerosPerCol_.size(); i++) {
    cout<<"("<<i<<", "<<numNonZerosPerCol_[i]<<", "<<numPairsExtraCoverage_[i]<<") ";
  }
}
  cout<<endl;

}

void Model2MasterLP::setMinPercentCoverage(double minCov)
{
  minPercentCoverage_=minCov;
}

bool Model2MasterLP::isThereEnoughCoverage(int rowsCovered)
{
  if(rowsCovered == 0)
    return false;

  if(rowsCovered < minPercentCoverage_ * n_)
    return false;

  return true;
}

void Model2MasterLP::addNumPairsExtraCoverage(int numPairsExtraCoverage)
{
  numPairsExtraCoverage_.push_back(numPairsExtraCoverage);
}

void Model2MasterLP::setObjPenaltyOnNumPairsExtraCoverage(double penalty)
{
  assert(x_.getSize() == xObjValues_.size());
  assert(x_.getSize() == numPairsExtraCoverage_.size());
  for (IloInt i = 0; i < x_.getSize(); i++) {
    double coef = xObjValues_[i];
    coef += numPairsExtraCoverage_[i]*(penalty-penaltyOnPairsExtraCoverage_);
    obj_.setLinearCoef(x_[i], coef);
    xObjValues_[i] = coef;
  }
  penaltyOnPairsExtraCoverage_ = penalty;
}

void Model2MasterLP::resetObjPenaltyOnNumPairsExtraCoverage()
{
  assert(x_.getSize() == xObjValues_.size());
  assert(x_.getSize() == numPairsExtraCoverage_.size());
  for (IloInt i = 0; i < x_.getSize(); i++) {
    double coef = xObjValues_[i];
    coef -= numPairsExtraCoverage_[i]*penaltyOnPairsExtraCoverage_;
    obj_.setLinearCoef(x_[i], coef);
    xObjValues_[i] = coef;
  }
  penaltyOnPairsExtraCoverage_ = 0.0;
}