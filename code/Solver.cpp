// © Copyright IBM Corporation 2022. All Rights Reserved.
// LICENSE: Eclipse Public License - v 2.0, https://opensource.org/licenses/EPL-2.0
// SPDX-License-Identifier: EPL-2.0

#include "Solver.hpp"

//#include <iostream>
#include <iomanip>
#include <list>

using namespace std;

Rankings::Rankings(int numTypes)
{
  rankingsRightRaw_.resize(numTypes);
  rankingsLeftRaw_.resize(numTypes);
  rankingsRightFiltered_.resize(numTypes);
  rankingsLeftFiltered_.resize(numTypes);
}

void Solver::setMinPercentCoverage(double minCov)
{
  minPercentCoverage_=minCov;
  cout<<"Minimum percent coverage to add column: "<<minPercentCoverage_<<endl;
}

void Solver::writeForwardChainGraph(string forwardChainFileName, string inputRulesFileName){
  data_.readDataForTesting(params_, "test.txt", "dummy.txt");
  ofstream outfile(forwardChainFileName.c_str());
  int numEntities = data_.getEntities().size();
  int numRelations = data_.getNumberRelations();
  rules_.resize(numRelations);
  rulesadded_.resize(numRelations);
  rulesselected_.resize(numRelations);
  rulesweights_.resize(numRelations);
  readRulesFromFile(inputRulesFileName);
  int numRules = 0;
  for (int i = 0; i<rulesselected_.size(); i++) {
    numRules += rulesselected_[i].size();
  }
  cout << "Number of rules " << numRules << endl; 
  int numTested = 0;


  for (int tail = 0; tail < numEntities; tail++) {
      for (int head = 0; head < numEntities; head++) {
          if (tail == head) continue;
          numTested++;

          pair <int,int> entityPair = std::make_pair(tail, head);
          double maxScore = params_.getMinForwardChainScore();
          vector<int> bestRelations;



          for (int rId = 0; rId < numRelations; rId++) {
              double score = getScore(rId, entityPair); // Uses learned rules
              if (score > maxScore) {
                  maxScore = score;
                  bestRelations.clear();
                  bestRelations.push_back(rId);
              } else if (score == maxScore) {
                  bestRelations.push_back(rId);
              }
          }

          // Write results to file (Part 1, Step 5)
          for (int rId : bestRelations) {
              outfile << data_.getEntities()[tail] << "\t" 
                      << data_.getRelations()[rId] << "\t" 
                      << data_.getEntities()[head] << endl;
          }
      }
  }

  cout << "Number of triples tested " << numTested << endl;
  
  outfile.close();
}

void Solver::run(string scoresFileName, string rulesFileName, string inputRulesFileName, string forwardChainFileName)
{
  srand(1234); // initialize random generator

  data_.readData(params_);

  if(params_.getUseFrequencyBias()){
    data_.computeRelationFrequencies();
  }

  bool runForReverseRelations = params_.getRunForReverseRelations();
  int numrelations = data_.getNumberRelations();
  int sizeRankings = numrelations;
  int timesToRunInnerLoop = 1;
  if(runForReverseRelations) {
    sizeRankings *= 2;
    timesToRunInnerLoop = 2;
  }
  maxComplexity_.resize(sizeRankings,params_.getMaxComplexity());
  rules_.resize(sizeRankings);
  rulesadded_.resize(sizeRankings);
  rulesselected_.resize(sizeRankings);
  rulesweights_.resize(sizeRankings);
  rulesspecificity_.resize(sizeRankings);
  rankings_.resize(sizeRankings, Rankings(5));
  rankingsAggressiveRightRaw_.resize(sizeRankings);
  rankingsAggressiveRightFiltered_.resize(sizeRankings);
  rankingsAggressiveLeftRaw_.resize(sizeRankings);
  rankingsAggressiveLeftFiltered_.resize(sizeRankings);
  rankingsMidPointRightRaw_.resize(sizeRankings);
  rankingsMidPointRightFiltered_.resize(sizeRankings);
  rankingsMidPointLeftRaw_.resize(sizeRankings);
  rankingsMidPointLeftFiltered_.resize(sizeRankings);
  rankingsRandomBreakRightRaw_.resize(sizeRankings);
  rankingsRandomBreakRightFiltered_.resize(sizeRankings);
  rankingsRandomBreakLeftRaw_.resize(sizeRankings);
  rankingsRandomBreakLeftFiltered_.resize(sizeRankings);
  rankingsRightRaw_.resize(sizeRankings);
  rankingsRightFiltered_.resize(sizeRankings);
  rankingsLeftRaw_.resize(sizeRankings);
  rankingsLeftFiltered_.resize(sizeRankings);

  rankingsAggressiveRelationRaw_.resize(sizeRankings);
  rankingsAggressiveRelationFiltered_.resize(sizeRankings);
  rankingsMidPointRelationRaw_.resize(sizeRankings);
  rankingsMidPointRelationFiltered_.resize(sizeRankings);
  rankingsRandomBreakRelationRaw_.resize(sizeRankings);
  rankingsRandomBreakRelationFiltered_.resize(sizeRankings);
  rankingsRelationRaw_.resize(sizeRankings);
  rankingsRelationFiltered_.resize(sizeRankings);

  int runMode = params_.getRunMode();
  if(runMode > 0)
    readRulesFromFile(inputRulesFileName);

  TestData& testdata = data_.getTestData();

    cout << "Building queries for all relations..." << endl;
  for(int r=0; r<data_.getNumberRelations(); r++) {
    data_.createQueryFromTrainingData(r);
  }
  cout << "All queries built." << endl;

  for(int relationId=0; relationId<numrelations; relationId++) {
    if(params_.getRunOnlyWithRelationId() && relationId != params_.getRelationId()) continue;

    if(params_.getRunOnlyForRelationsInTest()) {
      int numentitypairs = testdata.getNumEntityPairs(relationId);
      if(numentitypairs == 0) continue;
    }

    for(int iter=0; iter<timesToRunInnerLoop; iter++) {
      int modifiedRelationId = relationId + iter * numrelations;
      
      if(runMode != 1) { // if runMode==1 then read rules from file and write statistics
	if(runMode == 0) // if runMode>0, then read rules and so cannot clear
	  rules_[modifiedRelationId].clear();
  rulesadded_[modifiedRelationId].clear();
  rulesselected_[modifiedRelationId].clear();
  rulesweights_[modifiedRelationId].clear();
  
	  runOneRelation(modifiedRelationId);
      }
    writeRulesToFile(modifiedRelationId, rulesFileName);
    }
  }

  if (params_.getReportStatsEntity() || (params_.getReportStatsRelation() && runMode >0))
    if (params_.getTransductiveSetting()==false) {
      data_.clearGraphKeepRules();
      data_.readDataForTesting(params_, "test.txt", forwardChainFileName);
  }
    
  for(int relationId=0; relationId<numrelations; relationId++) {
    if(params_.getRunOnlyWithRelationId() && relationId != params_.getRelationId()) 
      continue; 
    if (params_.getReportStatsRelation()){
      for(int iter=0; iter<timesToRunInnerLoop; iter++) {
        int modifiedRelationId = relationId + iter * numrelations;
        writeRelationScoresToFile(modifiedRelationId, scoresFileName);
      }
    }
    else {
      writeScoresToFile(relationId, scoresFileName);
    }
  }

  if (params_.getReportStatsRelation() && runMode >0) {
    computeStatistics(scoresFileName, rankingsAggressiveRelationRaw_, rankingsMidPointRelationRaw_, rankingsRandomBreakRelationRaw_, rankingsRelationRaw_, false);
    computeStatistics(scoresFileName, rankingsAggressiveRelationFiltered_, rankingsMidPointRelationFiltered_, rankingsRandomBreakRelationFiltered_, rankingsRelationFiltered_, true);
  }

    if(params_.getReportStatsEntity()) {
      vector<vector<int> > rankingsAggressiveAllRaw(rankingsAggressiveRightRaw_.size()+rankingsAggressiveLeftRaw_.size());
      vector<vector<int> > rankingsAggressiveAllFiltered(rankingsAggressiveRightFiltered_.size()+rankingsAggressiveLeftFiltered_.size());
      vector<vector<int> > rankingsMidPointAllRaw(rankingsMidPointRightRaw_.size()+rankingsMidPointLeftRaw_.size());
      vector<vector<int> > rankingsMidPointAllFiltered(rankingsMidPointRightFiltered_.size()+rankingsMidPointLeftFiltered_.size());
      vector<vector<int> > rankingsRandomBreakAllRaw(rankingsRandomBreakRightRaw_.size()+rankingsRandomBreakLeftRaw_.size());
      vector<vector<int> > rankingsRandomBreakAllFiltered(rankingsRandomBreakRightFiltered_.size()+rankingsRandomBreakLeftFiltered_.size());
      vector<vector<int> > rankingsAllRaw(rankingsRightRaw_.size()+rankingsLeftRaw_.size());
      vector<vector<int> > rankingsAllFiltered(rankingsRightFiltered_.size()+rankingsLeftFiltered_.size());
  
    for(int i=0; i<(int)rankingsRightRaw_.size(); i++) {
      for(int j=0; j<(int)rankingsRightRaw_[i].size(); j++) {
	rankingsAggressiveAllRaw[i].push_back(rankingsAggressiveRightRaw_[i][j]);
	rankingsAggressiveAllFiltered[i].push_back(rankingsAggressiveRightFiltered_[i][j]);
	rankingsMidPointAllRaw[i].push_back(rankingsMidPointRightRaw_[i][j]);
	rankingsMidPointAllFiltered[i].push_back(rankingsMidPointRightFiltered_[i][j]);
	rankingsRandomBreakAllRaw[i].push_back(rankingsRandomBreakRightRaw_[i][j]);
	rankingsRandomBreakAllFiltered[i].push_back(rankingsRandomBreakRightFiltered_[i][j]);
	rankingsAllRaw[i].push_back(rankingsRightRaw_[i][j]);
	rankingsAllFiltered[i].push_back(rankingsRightFiltered_[i][j]);
      }
    }
    for(int i=0; i<(int)rankingsLeftRaw_.size(); i++) {
      for(int j=0; j<(int)rankingsLeftRaw_[i].size(); j++) {
	rankingsAggressiveAllRaw[i].push_back(rankingsAggressiveLeftRaw_[i][j]);
	rankingsAggressiveAllFiltered[i].push_back(rankingsAggressiveLeftFiltered_[i][j]);
	rankingsMidPointAllRaw[i].push_back(rankingsMidPointLeftRaw_[i][j]);
	rankingsMidPointAllFiltered[i].push_back(rankingsMidPointLeftFiltered_[i][j]);
	rankingsRandomBreakAllRaw[i].push_back(rankingsRandomBreakLeftRaw_[i][j]);
	rankingsRandomBreakAllFiltered[i].push_back(rankingsRandomBreakLeftFiltered_[i][j]);
	rankingsAllRaw[i].push_back(rankingsLeftRaw_[i][j]);
	rankingsAllFiltered[i].push_back(rankingsLeftFiltered_[i][j]);
      }
    }
    computeStatisticsForRelations(scoresFileName, rankingsAggressiveAllFiltered, rankingsMidPointAllFiltered, rankingsRandomBreakAllFiltered, rankingsAllFiltered, true);
    computeStatistics(scoresFileName, rankingsAggressiveAllRaw, rankingsMidPointAllRaw, rankingsRandomBreakAllRaw, rankingsAllRaw, false);
    computeStatistics(scoresFileName, rankingsAggressiveAllFiltered, rankingsMidPointAllFiltered, rankingsRandomBreakAllFiltered, rankingsAllFiltered, true);
  }  
}

void Solver::runOneRelation(int relationId)
{
  double objPenalty = params_.getPenaltyOnComplexity();
  bool addPenaltyOnNegativePairs = params_.getAddPenaltyOnNegativePairs();
  
  int numrelations = data_.getNumberRelations();
  if(relationId >= numrelations)
    cout<<"Relation R"<< relationId-numrelations << ": R_" << data_.getRelations()[relationId-numrelations]<<endl;
  else
    cout<<"Relation "<< relationId << ": " << data_.getRelations()[relationId]<<endl;

  assert(rules_[relationId].size() == 0);
  assert(rulesadded_[relationId].size() == 0);
  assert(rulesselected_[relationId].size() == 0);
  assert(rulesweights_[relationId].size() == 0);

    Model2MasterLP mlp = Model2MasterLP(relationId, data_, params_);

    int runMode = params_.getRunMode();
    if(runMode==0 || runMode==3) {
      int generateRulesType = params_.getGenerateRules();
      if(generateRulesType==0)
	generateRules(relationId, rules_[relationId]);
      else if(generateRulesType==1) {
	generateRulesS2(relationId, rules_[relationId]);
	generateRulesS0(relationId, rules_[relationId]);
      }
      else if(generateRulesType==2)
	generateRulesHeuristic(relationId, rules_[relationId]);
    }

    assert(rules_[relationId].size() > 0);
    mlp.setMinPercentCoverage(minPercentCoverage_);
    
      if(addPenaltyOnNegativePairs) {
	vector<int> column(data_.getNumPairsQuery(relationId));
	for(int i=0; i<(int)rules_[relationId].size(); i++) {
	  int numPairsExtraCov = getNumPairsExtraCoverage(relationId, rules_[relationId][i], column);
	  bool coladded = mlp.addCol(rules_[relationId][i], column, objPenalty);
	  if(coladded) {
	    mlp.addNumPairsExtraCoverage(numPairsExtraCov);
	    rulesadded_[relationId].push_back(i);
	  }
	}
      }
      else {
	for(int i=0; i<(int)rules_[relationId].size(); i++) {
	  bool coladded = mlp.addCol(rules_[relationId][i], objPenalty);
	  if(coladded) {
	    rulesadded_[relationId].push_back(i);
	  }
	}
      }
    
    cout<<"rules generated: "<<rules_[relationId].size()<<", rules added: "<<rulesadded_[relationId].size()<<", pairs in query: "<<data_.getNumPairsQuery(relationId)<<endl;
    if(addPenaltyOnNegativePairs) mlp.printLPStatistics();

    
    if (params_.getTransductiveSetting()) {
      mlp.setMaxComplexity(maxComplexity_[relationId]);
      setBestSettingsModel2(relationId, mlp);
    }
    else {
      mlp.setMaxComplexity(params_.getMaxComplexity());
      if (params_.getAddPenaltyOnNegativePairs())
        mlp.setObjPenaltyOnNumPairsExtraCoverage(params_.getPenaltyOnNegativePairs()[0]);

    }

    mlp.solveModel(params_.getWriteLpFile());
    mlp.getSolution(rulesselected_[relationId], rulesweights_[relationId]);

    if (params_.getUseSpecificityWeights()) {
    rulesspecificity_[relationId] = mlp.getRuleSpecificity();
}
      
  printSolution(relationId, true);
}

void Solver::setBestSettingsModel2(int relationId, Model2MasterLP& mlp)
{
  bool addPenaltyOnNegativePairs = params_.getAddPenaltyOnNegativePairs();
  vector<double>& objPenaltyNegPairs = params_.getPenaltyOnNegativePairs();

  if(addPenaltyOnNegativePairs && objPenaltyNegPairs.size()>1) {
    int bestComplexity;
    double bestPenalty;
    findBestComplexityAndPenalty(relationId,mlp,bestComplexity,bestPenalty);
    mlp.setMaxComplexity(bestComplexity);
    mlp.setObjPenaltyOnNumPairsExtraCoverage(bestPenalty);
  }
  else {
    assert(objPenaltyNegPairs.size()==1);
    if(addPenaltyOnNegativePairs && objPenaltyNegPairs[0]>0.0)
      mlp.setObjPenaltyOnNumPairsExtraCoverage(objPenaltyNegPairs[0]);
    if(params_.getRunFindBestComplexity()) {
      int bestComplexity = findBestComplexity(relationId, mlp);
      mlp.setMaxComplexity(bestComplexity);
    }
    else
      mlp.setMaxComplexity(maxComplexity_[relationId]);
  }

}

void Solver::printSolution(int relationId, bool printAll)
{
  int numRulesSelected = 0;
  int complexity = 0;
  cout<<"rules selected: ";
  vector<string>& relations = data_.getRelations();
  for(int i=0; i<(int)rulesselected_[relationId].size(); i++) {
    if(rulesselected_[relationId][i] > 0) {
      numRulesSelected++;
      complexity++;
      cout<<i<<"[";
      Rule& rule = rules_[relationId][rulesadded_[relationId][i]];
      int len = rule.getLengthRule();
      complexity += len;
      vector<int>& relIds = rule.getRelationIds();
      vector<bool>& isReverseArc = rule.getIsReverseArc();
      if(isReverseArc[0])
	cout<<"R"<<relIds[0];
      else
	cout<<relIds[0];
      for(int j=1; j<len; j++) {
	if(isReverseArc[j])
	  cout<<", R"<<relIds[j];
	else
	  cout<<", "<<relIds[j];
      }
      cout<<"] ";
    }
  }
  cout<<endl;

  
  cout<<"rules weights: ";

  for(int i=0; i<(int)rulesweights_[relationId].size(); i++) {
    if(rulesweights_[relationId][i] > 0) {
      if (params_.getUseSpecificityWeights()) {

    rulesweights_[relationId][i] *= rulesspecificity_[relationId][i];
  
}
      cout<<"("<<i<<", "<<rulesweights_[relationId][i]<<") ";
    }
  }
  cout<<endl;
  if(printAll) {
    cout<<"number rules added: "<<rulesadded_[relationId].size()<<endl;
    cout<<"number rules selected: "<<numRulesSelected<<endl;
    cout<<"complexity solution: "<<complexity<<endl;
  }
  cout<<"-------------------------"<<endl;  
}

int Solver::getNumPairsExtraCoverage(int modifiedRelationId, 
				     Rule& rule,
				     vector<int>& column)
{
  int numPairsExtraCov = 0;
  int numRelations = data_.getNumberRelations();
  int relationId = modifiedRelationId;
  bool isReverse = false;
  if(modifiedRelationId >= numRelations) {
    relationId = modifiedRelationId - numRelations;
    isReverse = true;
    cout<<"In getNumPairsExtraCoverage(...) isReverse is not implemented"<<endl;
    exit(1);
  }

  int n_pairs = data_.getNumPairsQuery(relationId);
  assert(n_pairs == column.size());

  int nGreaterZero = 0;
  for(int i=0; i<n_pairs; i++) {
    column[i] = 1;
    double basescore = getScore(relationId, rule, i);
    if(basescore > 0.0)
      nGreaterZero++;
    else
      column[i] = 0;
  }

  if(nGreaterZero <= minPercentCoverage_*n_pairs)
    return numPairsExtraCov; // the column has only zeros

  bool useBFS = params_.getUseBreadthFirstSearch();
  map<int,set<int> > rEntities, lEntities;
  data_.getEntitiesOfInterest(relationId, 0, rEntities, lEntities);

  int largeInt = 10000000;
  // remove right entities
  for(map<int,set<int> >::iterator it=rEntities.begin(); it!=rEntities.end() && numPairsExtraCov<largeInt; it++) {
    int origId = it->first;
    set<int> destIds;
    data_.getRightEntities(rule, origId, destIds, useBFS);
    set<int> diff;
    set_difference(destIds.begin(), destIds.end(), rEntities[origId].begin(), rEntities[origId].end(), inserter(diff, diff.end()));
    numPairsExtraCov += (int)(diff.size());
  }

  // remove left entities
  for(map<int,set<int> >::iterator it=lEntities.begin(); it!=lEntities.end() && numPairsExtraCov<largeInt; it++) {
    int destId = it->first;
    set<int> origIds;
    data_.getLeftEntities(rule, destId, origIds, useBFS);
    set<int> diff;
    set_difference(origIds.begin(), origIds.end(), lEntities[destId].begin(), lEntities[destId].end(), inserter(diff, diff.end()));
    numPairsExtraCov += (int)(diff.size());
  }

  return numPairsExtraCov;
}

double Solver::getScore(int relationId, Rule& rule, int cpairId)
{
  Query& query = data_.getQuery(relationId);
  vector<pair<int,int> >& pairs = query.getEntityPairs();
  vector<Arc*>& outArcsWithRelation = query.getOutArcsWithRelation();

  double score = 0.0;
  if(data_.hasPath(rule, pairs[cpairId], outArcsWithRelation[cpairId]))
    score = 1.0;
  return score;
}

double Solver::getScore(int relationId, pair<int,int>& cpair)
{
  double score = 0.0;
  for(int j=0; j<(int)rulesselected_[relationId].size(); j++) {
    if(rulesselected_[relationId][j] > 0) {
      Rule& rule = rules_[relationId][rulesadded_[relationId][j]];
      if(data_.hasPath(rule, cpair))
	score += rulesweights_[relationId][j];
      else if (params_.getUseRelaxedRules() && data_.hasRelaxedPath(rule, cpair)){
        int droppedrules = rule.getLengthRule() - 2;
        int exp_reducer = 1;
        for (int i=0; i<droppedrules; i++){
          exp_reducer *= 2;
        }
        score += rulesweights_[relationId][j] * params_.getRelaxedWeight()  / exp_reducer;
      }
    }
  }
  if(params_.getUseFrequencyBias()) {
    double bias = data_.getRelationFrequencyBias()[relationId];
    score += 0.009*bias + 0.001; // rescale bias to [0.001, 0.01]
  }
  return score;
}


void Solver::getEntitiesOfInterestForHead(int relationId, int tail, vector<bool>& useEntity, bool useAllData)
{
  int numEntities = (int)useEntity.size();
  for(int i=0; i<numEntities; i++)
    useEntity[i] = true;
  useEntity[tail] = false; // do not use the tail as head

  // entities from test dataset
  if(useAllData)
  {
    TestData& testdata = data_.getTestData();
    int n_pairs = testdata.getNumEntityPairs(relationId);
    vector<pair<int,int> >& entpairs = testdata.getEntityPairs(relationId);
    for(int i=0; i<n_pairs; i++) {
      pair<int,int>& cpair = entpairs[i];
      if(cpair.first == tail)
	useEntity[cpair.second] = false;
    }
  }

  // entities from valid dataset
  {
    TestData& validdata = data_.getValidData();
    int n_pairs = validdata.getNumEntityPairs(relationId);
    vector<pair<int,int> >& entpairs = validdata.getEntityPairs(relationId);
    for(int i=0; i<n_pairs; i++) {
      pair<int,int>& cpair = entpairs[i];
      if(cpair.first == tail)
	useEntity[cpair.second] = false;
    }
  }

  // entities from train dataset
  vector<vector<Arc*> >& allarcs = data_.getOutArcs();
  vector<Arc*>& arcs = allarcs[tail];
  for(int i=0; i<(int)arcs.size(); i++) {
    Arc* arc = arcs[i];
    if(arc->getIdRelation() == relationId)
      useEntity[arc->getHead()->getId()] = false;
  }

}

void Solver::getEntitiesOfInterestForTail(int relationId, int head, vector<bool>& useEntity, bool useAllData)
{
  int numEntities = (int)useEntity.size();
  for(int i=0; i<numEntities; i++)
    useEntity[i] = true;
  useEntity[head] = false; // do not use the head as tail

  // entities from test dataset
  if(useAllData)
  {
    TestData& testdata = data_.getTestData();
    int n_pairs = testdata.getNumEntityPairs(relationId);
    vector<pair<int,int> >& entpairs = testdata.getEntityPairs(relationId);
    for(int i=0; i<n_pairs; i++) {
      pair<int,int>& cpair = entpairs[i];
      if(cpair.second == head)
	useEntity[cpair.first] = false;
    }
  }

  // entities from valid dataset
  {
    TestData& validdata = data_.getValidData();
    int n_pairs = validdata.getNumEntityPairs(relationId);
    vector<pair<int,int> >& entpairs = validdata.getEntityPairs(relationId);
    for(int i=0; i<n_pairs; i++) {
      pair<int,int>& cpair = entpairs[i];
      if(cpair.second == head)
	useEntity[cpair.first] = false;
    }
  }

  // entities from train dataset
  vector<vector<Arc*> >& allarcs = data_.getInArcs();
  vector<Arc*>& arcs = allarcs[head];
  for(int i=0; i<(int)arcs.size(); i++) {
    Arc* arc = arcs[i];
    if(arc->getIdRelation() == relationId)
      useEntity[arc->getTail()->getId()] = false;
  }

}

bool Solver::shouldUpdateRanking(double basescore, double score, int rankingType)
{
  // 0 is aggresive, 1 is intermediate, 2 is conservative, 3 is randomBreak

  if((rankingType==0 || rankingType==4) && score>basescore)
    return true;

  if(rankingType==2 && score>=basescore) 
    return true;

  if(rankingType==1) {
    if((basescore>0.0 && score>basescore) ||
       (basescore==0.0 && score>=basescore))
      return true;
  }

  if(rankingType==3 && score>=basescore) {
    if(score>basescore)
      return true;
    else
      return (((double) rand() / (RAND_MAX))<0.5);
  }

  return false;
}

void Solver::getRightScores(Arc* outArcWithRelation, Rule& rule, int entityId, vector<double>& scores, bool useBFS)
{
  assert(data_.getEntities().size()==scores.size());
  for(int i=0; i<(int)scores.size(); i++)
    scores[i] = 0.0;

  set<int> destIds;
  data_.getRightEntities(outArcWithRelation, rule, entityId, destIds, useBFS);
  set<int>::iterator itr;
  for(itr = destIds.begin(); itr != destIds.end(); itr++)
    scores[*itr] += 1.0;
}

void Solver::getLeftScores(Arc* outArcWithRelation, Rule& rule, int entityId, vector<double>& scores, bool useBFS)
{
  assert(data_.getEntities().size()==scores.size());
  for(int i=0; i<(int)scores.size(); i++)
    scores[i] = 0.0;

  set<int> origIds;
  data_.getLeftEntities(outArcWithRelation, rule, entityId, origIds, useBFS);
  set<int>::iterator itr;
  for(itr = origIds.begin(); itr != origIds.end(); itr++)
    scores[*itr] += 1.0;
}

void Solver::getRightScores(int relationId, int entityId, vector<double>& scores, bool useBFS)
{
  assert(data_.getEntities().size()==scores.size());
  for(int i=0; i<(int)scores.size(); i++)
    scores[i] = 0.0;

  for(int j=0; j<(int)rulesselected_[relationId].size(); j++) {
    if(rulesselected_[relationId][j] > 0) {
      Rule& rule = rules_[relationId][rulesadded_[relationId][j]];
      set<int> destIds;
      data_.getRightEntities(rule, entityId, destIds, useBFS);
      for (int i=0; i<(int)scores.size(); i++){
        pair<int,int> cpair = pair<int,int>(entityId, i);
        if (destIds.count(i)>0)
          scores[i] += rulesweights_[relationId][j];
        else if (params_.getUseRelaxedRules() && data_.hasRelaxedPath(rule, cpair)){
        int droppedrules = rule.getLengthRule() - 2;
        int exp_reducer = 1;
        for (int i=0; i<droppedrules; i++){
          exp_reducer *= 2;
        }
        scores[i] += rulesweights_[relationId][j] * params_.getRelaxedWeight() / exp_reducer;
      

        }
      }
    }
  }

}

void Solver::getLeftScores(int relationId, int entityId, vector<double>& scores, bool useBFS)
{
  assert(data_.getEntities().size()==scores.size());
  for(int i=0; i<(int)scores.size(); i++)
    scores[i] = 0.0;

  for(int j=0; j<(int)rulesselected_[relationId].size(); j++) {
    if(rulesselected_[relationId][j] > 0) {
      Rule& rule = rules_[relationId][rulesadded_[relationId][j]];
      set<int> origIds;
      data_.getLeftEntities(rule, entityId, origIds, useBFS);
      for (int i=0; i<(int)scores.size(); i++){
        pair<int,int> cpair = pair<int,int>(i, entityId);
        if (origIds.count(i)>0)
          scores[i] += rulesweights_[relationId][j];
        else if (params_.getUseRelaxedRules() && data_.hasRelaxedPath(rule, cpair)){
        int droppedrules = rule.getLengthRule() - 2;
        int exp_reducer = 1;
        for (int i=0; i<droppedrules; i++){
          exp_reducer *= 2;
        }
        scores[i] += rulesweights_[relationId][j] * params_.getRelaxedWeight() / exp_reducer;
      

        }
      }
    }
  }

}

void Solver::getRelationScores(pair<int,int>& pair, vector<double>& scores, bool useBFS)
{
  int numRelations = data_.getNumberRelations();
  // For each candidate relation
  for(int candidateRelation=0; candidateRelation<numRelations; candidateRelation++) {
    scores[candidateRelation] = 0.0;
    if(params_.getUseFrequencyBias()) {
    double bias = data_.getRelationFrequencyBias()[candidateRelation];
    scores[candidateRelation] += 0.009*bias + 0.001; // rescale bias to [0.001, 0.01]
  }
    // For each rule of this relation
    for(int j=0; j<(int)rulesselected_[candidateRelation].size(); j++) {
      if(rulesselected_[candidateRelation][j] > 0) {
      Rule& rule = rules_[candidateRelation][rulesadded_[candidateRelation][j]];
      if(data_.hasPath(rule, pair))
	scores[candidateRelation] += rulesweights_[candidateRelation][j];
      else if (params_.getUseRelaxedRules() && data_.hasRelaxedPath(rule, pair)){
        int droppedrules = rule.getLengthRule() - 2;
        int exp_reducer = 1;
        for (int i=0; i<droppedrules; i++){
          exp_reducer *= 2;
        }
        scores[candidateRelation] += rulesweights_[candidateRelation][j] * params_.getRelaxedWeight() / exp_reducer;
      }
    }
  }
 }
}

void Solver::getRightScores(int relationId, vector<set<int> >& destIds, vector<double>& weights, map<int,double>& scores)
{
  for(int i=0; i<(int)destIds.size(); i++) {
    set<int>::iterator itr;
    for(itr = destIds[i].begin(); itr != destIds[i].end(); itr++) {
      map<int,double>::iterator it = scores.find(*itr);
      if(it != scores.end())
	scores[*itr] += weights[i];
      else
	scores[*itr] = weights[i];
    }
  }
}

void Solver::getLeftScores(int relationId, vector<set<int> >& origIds, vector<double>& weights, map<int,double>& scores)
{
  for(int i=0; i<(int)origIds.size(); i++) {
    set<int>::iterator itr;
    for(itr = origIds[i].begin(); itr != origIds[i].end(); itr++) {
      map<int,double>::iterator it = scores.find(*itr);
      if(it != scores.end())
	scores[*itr] += weights[i];
      else
	scores[*itr] = weights[i];
    }
  }
}

void Solver::getRightEntities(int relationId, int entityId, map<int,vector<set<int> > >& rDestIds, map<int,vector<double> >& rWeights, bool useBFS)
{
  map<int,vector<set<int> > >::iterator it = rDestIds.find(entityId);
  if(it == rDestIds.end()) {
    vector<set<int> > destIds;
    vector<double> weights;
    for(int j=0; j<(int)rulesselected_[relationId].size(); j++) {
      if(rulesselected_[relationId][j] > 0) {
	Rule& rule = rules_[relationId][rulesadded_[relationId][j]];
	set<int> tempDestIds;
	data_.getRightEntities(rule, entityId, tempDestIds, useBFS);
	if(tempDestIds.size()>0) {
	  destIds.push_back(tempDestIds);
	  weights.push_back(rulesweights_[relationId][j]);
	}
      }
    }
    rDestIds[entityId] = destIds;
    rWeights[entityId] = weights;
  }
}

void Solver::getLeftEntities(int relationId, int entityId, map<int,vector<set<int> > >& lOrigIds, map<int,vector<double> >& lWeights, bool useBFS)
{
  map<int,vector<set<int> > >::iterator it = lOrigIds.find(entityId);
  if(it == lOrigIds.end()) {
    vector<set<int> > origIds;
    vector<double> weights;
    for(int j=0; j<(int)rulesselected_[relationId].size(); j++) {
      if(rulesselected_[relationId][j] > 0) {
	Rule& rule = rules_[relationId][rulesadded_[relationId][j]];
	set<int> tempOrigIds;
	data_.getLeftEntities(rule, entityId, tempOrigIds, useBFS);
	if(tempOrigIds.size()>0) {
	  origIds.push_back(tempOrigIds);
	  weights.push_back(rulesweights_[relationId][j]);
	}
      }
    }
    lOrigIds[entityId] = origIds;
    lWeights[entityId] = weights;
  }
}

int Solver::getMidPointRank(int rankAggressive, int numSameScore)
{
  int rankMidPoint = numSameScore/2;
  if(numSameScore % 2 != 0 && (double) rand() / (RAND_MAX)<0.5)
    rankMidPoint += 1;
  rankMidPoint += rankAggressive;
  return rankMidPoint;
}

void Solver::writeScoresToFile(int modifiedRelationId, string fname)
{
  int numRelations = data_.getNumberRelations();
  int relationId = modifiedRelationId;
  bool isReverse = false;
  if(modifiedRelationId >= numRelations) {
    relationId = modifiedRelationId - numRelations;
    isReverse = true;
  }

  bool printScores = params_.getPrintScoresToFile();
  int rankingType = params_.getRankingType(); // 0 is aggresive, 1 is intermediate, 2 is conservative, 3 is randomBreak
  int aggressiveType = 0;
  int randomBreakType = 3;

  ofstream outfile(fname.c_str(), std::ios_base::app);

  vector<string>& relations = data_.getRelations();
  if(printScores) {
    outfile<<"NOTE: A score followed by * means that"<<endl;
    outfile<<"it is not included in the filtered rankings"<<endl;
    outfile<<"---------------------------------"<<endl;
    if(isReverse)
      outfile<<"Relation: R_"<<relations[relationId]<<endl;
    else
      outfile<<"Relation: "<<relations[relationId]<<endl;
    outfile<<"---------------------------------"<<endl;
  }

  vector<string>& entities = data_.getEntities();

  TestData& testdata = data_.getTestData();
  int n_pairs = testdata.getNumEntityPairs(relationId);
  vector<pair<int,int> >& entpairs = testdata.getEntityPairs(relationId);

  bool useBFS = params_.getUseBreadthFirstSearch();
  vector<double> scores((int)entities.size());
  vector<bool> useRightEntity((int)entities.size());
  vector<bool> useLeftEntity((int)entities.size());

  numRuleFired_ = 0;

  if(n_pairs == 0) {
    cout << "SKIPPING: No test pairs!" << endl;
    return;
  }

  for(int i=0; i<n_pairs; i++) {
    pair<int,int>& tempcpair = entpairs[i];
    pair<int,int> cpair;
    if(isReverse) {
      cpair = pair<int,int>(tempcpair.second, tempcpair.first);
      getEntitiesOfInterestForTail(relationId, cpair.first, useRightEntity);
      getEntitiesOfInterestForHead(relationId, cpair.second, useLeftEntity);
    }
    else {
      cpair = pair<int,int>(tempcpair.first, tempcpair.second);
      getEntitiesOfInterestForHead(relationId, cpair.first, useRightEntity);
      getEntitiesOfInterestForTail(relationId, cpair.second, useLeftEntity);
    }
    double basescore = getScore(relationId, cpair);
    if(printScores)
      outfile<<entities[cpair.first]<<" "
	     <<entities[cpair.second]<<" "
	     <<basescore<<endl<<endl;
    int rankAggressiveRightRaw = 1;
    int rankAggressiveRightFiltered = 1;
    int rankAggressiveLeftRaw = 1;
    int rankAggressiveLeftFiltered = 1;
    int rankRandomBreakRightRaw = 1;
    int rankRandomBreakRightFiltered = 1;
    int rankRandomBreakLeftRaw = 1;
    int rankRandomBreakLeftFiltered = 1;
    int rankRightRaw = 1;
    int rankRightFiltered = 1;
    int rankLeftRaw = 1;
    int rankLeftFiltered = 1;
    int numSameScoreRightRaw = 0;
    int numSameScoreRightFiltered = 0;
    int numSameScoreLeftRaw = 0;
    int numSameScoreLeftFiltered = 0;

      int origId = cpair.first;
      getRightScores(relationId, origId, scores, useBFS);
      assert(basescore == scores[cpair.second]);
      if (basescore > 0.0) {
      numRuleFired_++;
    }
      for(int k=0; k<(int)entities.size(); k++) {
	if(k != cpair.first && k != cpair.second) {
	  double score = scores[k];
	  if(score==basescore) {
	    numSameScoreRightRaw++;
	    if(useRightEntity[k])
	      numSameScoreRightFiltered++;
	  }
	  if(shouldUpdateRanking(basescore, score, aggressiveType)) {
	    rankAggressiveRightRaw++;
	    if(useRightEntity[k])
	      rankAggressiveRightFiltered++;
	  }
	  if(shouldUpdateRanking(basescore, score, randomBreakType)) {
	    rankRandomBreakRightRaw++;
	    if(useRightEntity[k])
	      rankRandomBreakRightFiltered++;
	  }
	  if(shouldUpdateRanking(basescore, score, rankingType)) {
	    rankRightRaw++;
	    if(useRightEntity[k])
	      rankRightFiltered++;
	  }
	  if(printScores && score > 0.0) {
	    if(useRightEntity[k])
	      outfile<<entities[origId]<<" "
		     <<entities[k]<<" "
		     <<score<<endl;
	    else
	      outfile<<entities[origId]<<" "
		     <<entities[k]<<" "
		     <<score<<"*"<<endl;
	  }
	}
      }

      int destId = cpair.second;
      getLeftScores(relationId, destId, scores, useBFS);
      assert(basescore == scores[cpair.first]);
      for(int k=0; k<(int)entities.size(); k++) {
	if(k != cpair.first && k != cpair.second) {
	  double score = scores[k];
	  if(score==basescore) {
	    numSameScoreLeftRaw++;
	    if(useLeftEntity[k])
	      numSameScoreLeftFiltered++;
	  }
	  if(shouldUpdateRanking(basescore, score, aggressiveType)) {
	    rankAggressiveLeftRaw++;
	    if(useLeftEntity[k])
	      rankAggressiveLeftFiltered++;
	  }
	  if(shouldUpdateRanking(basescore, score, randomBreakType)) {
	    rankRandomBreakLeftRaw++;
	    if(useLeftEntity[k])
	      rankRandomBreakLeftFiltered++;
	  }
	  if(shouldUpdateRanking(basescore, score, rankingType)) {
	    rankLeftRaw++;
	    if(useLeftEntity[k])
	      rankLeftFiltered++;
	  }
	  if(printScores && score > 0.0) {
	    if(useLeftEntity[k])
	      outfile<<entities[k]<<" "
		     <<entities[destId]<<" "
		     <<score<<endl;
	    else
	      outfile<<entities[k]<<" "
		     <<entities[destId]<<" "
		     <<score<<"*"<<endl;
	  }
	}
      }

      rankingsAggressiveRightRaw_[modifiedRelationId].push_back(rankAggressiveRightRaw);
      rankingsAggressiveRightFiltered_[modifiedRelationId].push_back(rankAggressiveRightFiltered);
      rankingsMidPointRightRaw_[modifiedRelationId].push_back(getMidPointRank(rankAggressiveRightRaw,numSameScoreRightRaw));
      rankingsMidPointRightFiltered_[modifiedRelationId].push_back(getMidPointRank(rankAggressiveRightFiltered,numSameScoreRightFiltered));
      rankingsRandomBreakRightRaw_[modifiedRelationId].push_back(rankRandomBreakRightRaw);
      rankingsRandomBreakRightFiltered_[modifiedRelationId].push_back(rankRandomBreakRightFiltered);
      rankingsRightRaw_[modifiedRelationId].push_back(rankRightRaw);
      rankingsRightFiltered_[modifiedRelationId].push_back(rankRightFiltered);

      rankingsAggressiveLeftRaw_[modifiedRelationId].push_back(rankAggressiveLeftRaw);
      rankingsAggressiveLeftFiltered_[modifiedRelationId].push_back(rankAggressiveLeftFiltered);
      rankingsMidPointLeftRaw_[modifiedRelationId].push_back(getMidPointRank(rankAggressiveLeftRaw,numSameScoreLeftRaw));
      rankingsMidPointLeftFiltered_[modifiedRelationId].push_back(getMidPointRank(rankAggressiveLeftFiltered,numSameScoreLeftFiltered));
      rankingsRandomBreakLeftRaw_[modifiedRelationId].push_back(rankRandomBreakLeftRaw);
      rankingsRandomBreakLeftFiltered_[modifiedRelationId].push_back(rankRandomBreakLeftFiltered);
      rankingsLeftRaw_[modifiedRelationId].push_back(rankLeftRaw);
      rankingsLeftFiltered_[modifiedRelationId].push_back(rankLeftFiltered);
    if(printScores)
      outfile<<"---------------------------------"<<endl;
  }

  outfile.close();
}

void Solver::writeRelationScoresToFile(int modifiedRelationId, string fname)
{
  int numRelations = data_.getNumberRelations();
  int relationId = modifiedRelationId;
  bool isReverse = false;
  if(modifiedRelationId >= numRelations) {
    relationId = modifiedRelationId - numRelations;
    isReverse = true;
  }  

  bool printScores = params_.getPrintScoresToFile();
  int rankingType = params_.getRankingType();
  int aggressiveType = 0;
  int randomBreakType = 3;

  ofstream outfile(fname.c_str(), std::ios_base::app);

  vector<string>& relations = data_.getRelations();
  if(printScores) {
    outfile<<"NOTE: A score followed by * means that"<<endl;
    outfile<<"it is not included in the filtered rankings"<<endl;
    outfile<<"---------------------------------"<<endl;
    if(isReverse)
      outfile<<"Relation: R_"<<relations[relationId]<<endl;
    else
      outfile<<"Relation: "<<relations[relationId]<<endl;
    outfile<<"---------------------------------"<<endl;
  }

  vector<string>& entities = data_.getEntities();

  TestData& testdata = data_.getTestData();
  int n_pairs = testdata.getNumEntityPairs(relationId);
  vector<pair<int,int> >& entpairs = testdata.getEntityPairs(relationId);

  numRuleFired_ = 0;
 
  if(n_pairs == 0) {
    cout << "SKIPPING: No test pairs!" << endl;
    return;
  }

  bool useBFS = params_.getUseBreadthFirstSearch();
  vector<double> scores(numRelations);
  vector<bool> useRelation(numRelations);


  for(int i=0; i<n_pairs; i++) {
    pair<int,int>& tempcpair = entpairs[i];
    pair<int,int> cpair;
    if(isReverse) {
      cpair = pair<int,int>(tempcpair.second, tempcpair.first);
    }
    else {
      cpair = pair<int,int>(tempcpair.first, tempcpair.second);
    }

    int head = cpair.second;
    int tail = cpair.first;
    int trueRelation = relationId;

    // Determine which relations to include in filtered ranking
for(int r=0; r<numRelations; r++) {
  useRelation[r] = true;
}

// Exclude relations that connect (head, tail) in TRAINING data
vector<vector<Arc*> >& allarcs  = data_.getInArcs();
vector<Arc*>& arcs = allarcs[head];
for(int i=0; i<(int)arcs.size(); i++) {
  if(arcs[i]->getTail()->getId() == tail) {
    int trainRelation = arcs[i]->getIdRelation();
    if(trainRelation != trueRelation) {
      useRelation[trainRelation] = false;
    }
  }
}

// Exclude from VALID data
TestData& validdata = data_.getValidData();
for(int r=0; r<numRelations; r++) {
  if(r == trueRelation) continue;
  vector<pair<int,int> >& validPairs = validdata.getEntityPairs(r);
  for(int i=0; i<(int)validPairs.size(); i++) {
    if(validPairs[i].first == tail && validPairs[i].second == head) {
      useRelation[r] = false;
    }
  }
}

// Exclude from OTHER TEST relations
for(int r=0; r<numRelations; r++) {
  if(r == trueRelation) continue;
  vector<pair<int,int> >& testPairs = testdata.getEntityPairs(r);
  for(int i=0; i<(int)testPairs.size(); i++) {
    if(testPairs[i].first == tail && testPairs[i].second == head) {
      useRelation[r] = false;
    }
  }
}
    double basescore = getScore(relationId, cpair);

    if(printScores)
      outfile<<entities[head]<<" "
             <<relations[trueRelation]<<" "
             <<entities[tail]<<" "
             <<basescore<<endl<<endl;

    int rankAggressiveRaw = 1;
    int rankAggressiveFiltered = 1;
    int rankRandomBreakRaw = 1;
    int rankRandomBreakFiltered = 1;
    int rankRaw = 1;
    int rankFiltered = 1;
    int numSameScoreRaw = 0;
    int numSameScoreFiltered = 0;

    
    getRelationScores(cpair,scores,useBFS);
    assert(basescore == scores[trueRelation]);
    if (basescore > 0.0) {
      numRuleFired_++;
    }
    for(int r=0; r<numRelations; r++) {
      if(r != trueRelation) {
        double score = scores[r];
        if(score == basescore) {
          numSameScoreRaw++;
          if(useRelation[r])
            numSameScoreFiltered++;
        }
        if(shouldUpdateRanking(basescore, score, aggressiveType)) {
          rankAggressiveRaw++;
          if(useRelation[r])
            rankAggressiveFiltered++;
        }
        if(shouldUpdateRanking(basescore, score, randomBreakType)) {
          rankRandomBreakRaw++;
          if(useRelation[r])
            rankRandomBreakFiltered++;
        }
        if(shouldUpdateRanking(basescore, score, rankingType)) {
          rankRaw++;
          if(useRelation[r])
            rankFiltered++;
        }
        if(printScores && score > 0.0) {
          if(useRelation[r])
            outfile<<entities[head]<<" "
                   <<relations[r]<<" "
                   <<entities[tail]<<" "
                   <<score<<endl;
          else
            outfile<<entities[head]<<" "
                   <<relations[r]<<" "
                   <<entities[tail]<<" "
                   <<score<<"*"<<endl;
        }
      }
      
    }

    rankingsAggressiveRelationRaw_[modifiedRelationId].push_back(rankAggressiveRaw);
    rankingsAggressiveRelationFiltered_[modifiedRelationId].push_back(rankAggressiveFiltered);
    rankingsMidPointRelationRaw_[modifiedRelationId].push_back(getMidPointRank(rankAggressiveRaw, numSameScoreRaw));
    rankingsMidPointRelationFiltered_[modifiedRelationId].push_back(getMidPointRank(rankAggressiveFiltered, numSameScoreFiltered));
    rankingsRandomBreakRelationRaw_[modifiedRelationId].push_back(rankRandomBreakRaw);
    rankingsRandomBreakRelationFiltered_[modifiedRelationId].push_back(rankRandomBreakFiltered);
    rankingsRelationRaw_[modifiedRelationId].push_back(rankRaw);
    rankingsRelationFiltered_[modifiedRelationId].push_back(rankFiltered);

    if(printScores)
      outfile<<"---------------------------------"<<endl;
  }

  outfile.close();
}

void Solver::findBestComplexityAndPenalty(int modifiedRelationId,
					  Model2MasterLP& mlp,
					  int& bestComplexity,
					  double& bestPenalty)
{
  vector<double>& objPenaltyNegPairs = params_.getPenaltyOnNegativePairs();

  int numRelations = data_.getNumberRelations();
  int relationId = modifiedRelationId;
  bool isReverse = false;
  if(modifiedRelationId >= numRelations) {
    relationId = modifiedRelationId - numRelations;
    isReverse = true;
  }
  int numEntities = (int)(data_.getEntities().size());

  int rankingType = params_.getFindBestComplexityRankingType(); // 0 is aggresive, 1 is intermediate, 2 is conservative, 3 is randomBreak
  int aggressiveType = 0;
  int randomBreakType = 3;

  bool useBFS = params_.getUseBreadthFirstSearch();
  map<int,set<int> > rEntities, lEntities;
  data_.getEntitiesOfInterest(relationId, 1, rEntities, lEntities);

  int startComplexity = maxComplexity_[relationId];
  bestComplexity = startComplexity;
  double bestMetric = 0.0;
  bestPenalty = 0.0;

  for(int ipen=0; ipen<(int)objPenaltyNegPairs.size(); ipen++) {
    double currentPenalty = objPenaltyNegPairs[ipen];
    mlp.setObjPenaltyOnNumPairsExtraCoverage(currentPenalty);

    assert(rulesselected_[relationId].size()==0);
    assert(rulesweights_[relationId].size()==0);

    int initialComplexity = startComplexity;
    int bestLocalComplexity = 0;
    int currentComplexity = 0;
    double bestLocalMetric = 0.0;
    int maxIter = 20;
    if(!params_.getRunFindBestComplexity())
      maxIter = 1;
    int iter = 0;
    bool keepGoing = true;
    while(iter<maxIter && keepGoing) {
      currentComplexity += initialComplexity;
      cout<<"Iter: "<<iter<<", Trying penalty: "<<currentPenalty<<", Trying complexity: "<<currentComplexity<<endl;
      mlp.setMaxComplexity(currentComplexity);
      mlp.solveModel(params_.getWriteLpFile());
      mlp.getSolution(rulesselected_[relationId], rulesweights_[relationId]);

      vector<int> rankingsFiltered;
      vector<string>& entities = data_.getEntities();
      TestData& validdata = data_.getValidData();
      int n_pairs = validdata.getNumEntityPairs(relationId);
      vector<pair<int,int> >& entpairs = validdata.getEntityPairs(relationId);
      map<int,vector<set<int> > > rDestIds;
      map<int,vector<double> > rWeights;
      map<int,vector<set<int> > > lOrigIds;
      map<int,vector<double> > lWeights;

        int numNegativePairs = 0;
    if (params_.getUseSoftMargin()) {

      double negativeWeights = 0.0;
      double total_spec = 0.0;
      int counter = 0;

        for(int r=0; r<numRelations; r++) {
    if(r == relationId) continue;  
    
    vector<pair<int,int> >& entpairs = validdata.getEntityPairs(r);
    
    int numPairsToSample = min(10, (int)entpairs.size());
    
    for(int i=0; i<numPairsToSample; i++) {
      int idx = rand() % entpairs.size();
      numNegativePairs++;

      for(int j=0; j<rulesselected_[relationId].size(); j++) {
    if (rulesselected_[relationId][j] >0) {
      Rule& rule = rules_[relationId][rulesadded_[relationId][j]];
      if(data_.hasPath(rule, entpairs[idx])) 
        negativeWeights += rulesweights_[relationId][j];

      total_spec += 1.0 - negativeWeights / numNegativePairs;
      counter++;
    }
  }
    }
  }
        double avg_spec = total_spec/counter;
        if(total_spec > bestLocalMetric) {
    bestLocalMetric = total_spec;
    bestLocalComplexity = currentComplexity;
      }
        if (avg_spec < 0.9)
          keepGoing = false;

      cout<<"Total Specificity: "<<total_spec<<", Avg Specificity: "<<avg_spec<<", bestMetric: "<<bestLocalMetric<<", currentComplexity: "<<currentComplexity<<", bestComplexity: "<<bestLocalComplexity<<endl;

    }

  else {
      for(int i=0; i<n_pairs; i++) {
	pair<int,int>& tempcpair = entpairs[i];
	pair<int,int> cpair;
	if(isReverse) {
	  cpair = pair<int,int>(tempcpair.second, tempcpair.first);
	  cout<<"The Reverse is not working"<<endl;
	  exit(1);
	}
	else {
	  cpair = pair<int,int>(tempcpair.first, tempcpair.second);
	}
	double basescore = getScore(relationId, cpair);
	int rankRightFiltered = 1;
	int rankLeftFiltered = 1;


  if(basescore > 0.0) {
	    int origId = cpair.first;
	    getRightEntities(relationId, origId, rDestIds, rWeights, useBFS);
	    if(rDestIds[origId].size()>0) {
	      vector<set<int> > diff(rDestIds[origId].size());
	      for(int j=0; j<(int)rDestIds[origId].size(); j++) {
		set_difference(rDestIds[origId][j].begin(), rDestIds[origId][j].end(), rEntities[origId].begin(), rEntities[origId].end(), inserter(diff[j], diff[j].end()));
	      }
	      map<int,double> scores;
	      getRightScores(relationId, diff, rWeights[origId], scores);
	      map<int,double>::iterator it;
	      for(it = scores.begin(); it != scores.end(); it++) {
		int k = it->first;
		if(k != cpair.first && k != cpair.second) {
		  double score = it->second;
		  if(shouldUpdateRanking(basescore, score, rankingType))
		    rankRightFiltered++;
		}
	      }
	    }
	  }
	  else
	    rankRightFiltered = numEntities;

	  if(basescore > 0.0) {
	    int destId = cpair.second;
	    getLeftEntities(relationId, destId, lOrigIds, lWeights, useBFS);
	    if(lOrigIds[destId].size()>0) {
	      vector<set<int> > diff(lOrigIds[destId].size());
	      for(int j=0; j<(int)lOrigIds[destId].size(); j++) {
		set_difference(lOrigIds[destId][j].begin(), lOrigIds[destId][j].end(), lEntities[destId].begin(), lEntities[destId].end(), inserter(diff[j], diff[j].end()));
	      }
	      map<int,double> scores;
	      getLeftScores(relationId, diff, lWeights[destId], scores);
	      map<int,double>::iterator it;
	      for(it = scores.begin(); it != scores.end(); it++) {
		int k = it->first;
		if(k != cpair.first && k != cpair.second) {
		  double score = it->second;
		  if(shouldUpdateRanking(basescore, score, rankingType))
		    rankLeftFiltered++;
		}
	      }
	    }
	  }
	  else
	    rankLeftFiltered = numEntities;
	

	  rankingsFiltered.push_back(rankRightFiltered);
	
	  rankingsFiltered.push_back(rankLeftFiltered);
	
      }

      

      double mrr = computeMRR(rankingsFiltered);
      if(mrr > bestLocalMetric) {
	bestLocalMetric = mrr;
	bestLocalComplexity = currentComplexity;
      }

      cout<<"MRR: "<<mrr<<", bestMetric: "<<bestLocalMetric<<", currentComplexity: "<<currentComplexity<<", bestComplexity: "<<bestLocalComplexity<<endl;

  }

      rulesselected_[relationId].clear();
      rulesweights_[relationId].clear();
      if(rankingsFiltered.size()==0 && numNegativePairs==0) {
	cout<<"This relation does not exist in valid.txt"<<endl;
	break;
      }

      iter++;
    }

    if(bestLocalMetric > bestMetric) {
      bestMetric = bestLocalMetric;
      bestComplexity = bestLocalComplexity;
      bestPenalty = currentPenalty;
    }
    cout<<"End of Penalty "<<currentPenalty<<", bestMetric: "<<bestMetric<<", bestComplexity: "<<bestComplexity<<", bestPenalty: "<<bestPenalty<<endl;

  }
  cout<<"End of findBestComplexityAndPenalty. bestMetric: "<<bestMetric<<", bestComplexity: "<<bestComplexity<<", bestPenalty: "<<bestPenalty<<endl;
}

int Solver::findBestComplexity(int modifiedRelationId,
			       Model2MasterLP& mlp)
{
  bool useBFS = params_.getUseBreadthFirstSearch();

  int numRelations = data_.getNumberRelations();
  int relationId = modifiedRelationId;
  bool isReverse = false;
  if(modifiedRelationId >= numRelations) {
    relationId = modifiedRelationId - numRelations;
    isReverse = true;
  }

  bool printScores = params_.getPrintScoresToFile();
  int rankingType = params_.getFindBestComplexityRankingType(); // 0 is aggresive, 1 is intermediate, 2 is conservative, 3 is randomBreak
  int aggressiveType = 0;
  int randomBreakType = 3;

  assert(rulesselected_[relationId].size()==0);
  assert(rulesweights_[relationId].size()==0);

  int initialComplexity = maxComplexity_[relationId];
  int bestComplexity = initialComplexity;
  int currentComplexity = 0;
  double bestMetric = 0.0;
  int maxIter = 20;
  int iter = 0;
  bool keepGoing = true;

  while(iter<maxIter&& keepGoing) {
    currentComplexity += initialComplexity;
    cout<<"Iter: "<<iter<<", Trying complexity: "<<currentComplexity<<endl;
    mlp.setMaxComplexity(currentComplexity);
    mlp.solveModel(params_.getWriteLpFile());
    mlp.getSolution(rulesselected_[relationId], rulesweights_[relationId]);

    vector<int> rankingsFiltered;
    vector<string>& entities = data_.getEntities();
    TestData& validdata = data_.getValidData();
    int n_pairs = validdata.getNumEntityPairs(relationId);
    vector<pair<int,int> >& entpairs = validdata.getEntityPairs(relationId);
    vector<double> scores((int)entities.size());
    vector<bool> useRightEntity((int)entities.size());
    vector<bool> useLeftEntity((int)entities.size());

       int numNegativePairs = 0;
    if (params_.getUseSoftMargin()) {

      double negativeWeights = 0.0;
      double total_spec = 0.0;
      int counter = 0;

        for(int r=0; r<numRelations; r++) {
    if(r == relationId) continue;  
    
    vector<pair<int,int> >& entpairs = validdata.getEntityPairs(r);
    
    int numPairsToSample = min(10, (int)entpairs.size());
    
    for(int i=0; i<numPairsToSample; i++) {
      int idx = rand() % entpairs.size();
      numNegativePairs++;

      for(int j=0; j<rulesselected_[relationId].size(); j++) {
    if (rulesselected_[relationId][j] >0) {
      Rule& rule = rules_[relationId][rulesadded_[relationId][j]];
      if(data_.hasPath(rule, entpairs[idx])) 
        negativeWeights += rulesweights_[relationId][j];

      total_spec += 1.0 - negativeWeights / numNegativePairs;
      counter++;
    }
  }
    }
  }
        double avg_spec = total_spec/counter;
        if(total_spec > bestMetric) {
    bestMetric = total_spec;
    bestComplexity = currentComplexity;
      }
        if (avg_spec < 0.9)
          keepGoing = false;

      cout<<"Total Specificity: "<<total_spec<<", Avg Specificity: "<<avg_spec<<", bestMetric: "<<bestMetric<<", currentComplexity: "<<currentComplexity<<", bestComplexity: "<<bestComplexity<<endl;

    }

  else {
    for(int i=0; i<n_pairs; i++) {
      pair<int,int>& tempcpair = entpairs[i];
      pair<int,int> cpair;
      if(isReverse) {
	cpair = pair<int,int>(tempcpair.second, tempcpair.first);
	getEntitiesOfInterestForTail(relationId, cpair.first, useRightEntity);
	getEntitiesOfInterestForHead(relationId, cpair.second, useLeftEntity);
      }
      else {
	cpair = pair<int,int>(tempcpair.first, tempcpair.second);
	getEntitiesOfInterestForHead(relationId, cpair.first, useRightEntity);
	getEntitiesOfInterestForTail(relationId, cpair.second, useLeftEntity);
      }
      double basescore = getScore(relationId, cpair);
      int rankAggressiveRightRaw = 1;
      int rankAggressiveRightFiltered = 1;
      int rankAggressiveLeftRaw = 1;
      int rankAggressiveLeftFiltered = 1;
      int rankRandomBreakRightRaw = 1;
      int rankRandomBreakRightFiltered = 1;
      int rankRandomBreakLeftRaw = 1;
      int rankRandomBreakLeftFiltered = 1;
      int rankRightRaw = 1;
      int rankRightFiltered = 1;
      int rankLeftRaw = 1;
      int rankLeftFiltered = 1;

	int origId = cpair.first;
	getRightScores(relationId, origId, scores, useBFS);
	assert(basescore == scores[cpair.second]);
	for(int k=0; k<(int)entities.size(); k++) {
	  if(k != cpair.first && k != cpair.second) {
	    double score = scores[k];
	    if(shouldUpdateRanking(basescore, score, rankingType)) {
	      rankRightRaw++;
	      if(useRightEntity[k])
		rankRightFiltered++;
	    }
	  }
	}
      

	int destId = cpair.second;
	getLeftScores(relationId, destId, scores, useBFS);
	assert(basescore == scores[cpair.first]);
	for(int k=0; k<(int)entities.size(); k++) {
	  if(k != cpair.first && k != cpair.second) {
	    double score = scores[k];

	    if(shouldUpdateRanking(basescore, score, rankingType)) {
	      rankLeftRaw++;
	      if(useLeftEntity[k])
		rankLeftFiltered++;
	    }
	  }
	}
      

	rankingsFiltered.push_back(rankRightFiltered);
	rankingsFiltered.push_back(rankLeftFiltered);
      
    }

     double mrr = computeMRR(rankingsFiltered);
    if(mrr >= bestMetric) {
      bestMetric = mrr;
      bestComplexity = currentComplexity;
    }
    cout<<"MRR: "<<mrr<<", bestMetric: "<<bestMetric<<", currentComplexity: "<<currentComplexity<<", bestComplexity: "<<bestComplexity<<endl;
   

  }

    rulesselected_[relationId].clear();
    rulesweights_[relationId].clear();

    // if the relation doesn't exist in valid.txt, exit
    if(rankingsFiltered.size()==0 && numNegativePairs==0) {
      cout<<"This relation does not exist in valid.txt"<<endl;
      break;
    }

    iter++;
  }

  return bestComplexity;
}

double Solver::computeMRR(vector<int>& rankings)
{
  double mrr = 0.0;
  int counter = 0;
  for(int i=0; i<(int)rankings.size(); i++) {
    counter++;
    int rank = rankings[i];
    mrr += 1.0 / rank;
  }
  if(counter > 0)
    mrr /= counter;
  return mrr;
}

void Solver::computeStatistics(string fname, vector<vector<int> >& rankingsAggressive, vector<vector<int> >& rankingsMidPoint, vector<vector<int> >& rankingsRandomBreak, vector<vector<int> >& rankings, bool isFiltered)
{
  ofstream outfile(fname.c_str(), std::ios_base::app);

  vector<string>& relations = data_.getRelations();

  double aggressivehits1 = 0.0;
  double aggressivehits3 = 0.0;
  double aggressivehits5 = 0.0;
  double aggressivehits10 = 0.0;
  double aggressivehits20 = 0.0;
  double aggressivemrr = 0.0;
  double aggressivemr = 0.0;
  double midpointhits1 = 0.0;
  double midpointhits3 = 0.0;
  double midpointhits5 = 0.0;
  double midpointhits10 = 0.0;
  double midpointhits20 = 0.0;
  double midpointmrr = 0.0;
  double midpointmr = 0.0;
  double randombreakhits1 = 0.0;
  double randombreakhits3 = 0.0;
  double randombreakhits5 = 0.0;
  double randombreakhits10 = 0.0;
  double randombreakhits20 = 0.0;
  double randombreakmrr = 0.0;
  double randombreakmr = 0.0;
  double hits1 = 0.0;
  double hits3 = 0.0;
  double hits5 = 0.0;
  double hits10 = 0.0;
  double hits20 = 0.0;
  double mrr = 0.0;
  double mr = 0.0;

  int counter = 0;
  for(int i=0; i<(int)rankings.size(); i++) {
    for(int j=0; j<(int)rankings[i].size(); j++) {
      counter++;

      int rank = rankingsAggressive[i][j];
      if(rank <= 20) {
	aggressivehits20 += 1;
	if(rank <= 10) {
	  aggressivehits10 += 1;
	  if(rank <= 5) {
	    aggressivehits5 += 1;
	    if(rank <= 3) {
	      aggressivehits3 += 1;
	      if(rank == 1) {
		aggressivehits1 += 1;
	      }
	    }
	  }
	}
      }
      aggressivemrr += 1.0 / rank;
      aggressivemr += (double)rank;

      rank = rankingsMidPoint[i][j];
      if(rank <= 20) {
	midpointhits20 += 1;
	if(rank <= 10) {
	  midpointhits10 += 1;
	  if(rank <= 5) {
	    midpointhits5 += 1;
	    if(rank <= 3) {
	      midpointhits3 += 1;
	      if(rank == 1) {
		midpointhits1 += 1;
	      }
	    }
	  }
	}
      }
      midpointmrr += 1.0 / rank;
      midpointmr += (double)rank;

      rank = rankingsRandomBreak[i][j];
      if(rank <= 20) {
	randombreakhits20 += 1;
	if(rank <= 10) {
	  randombreakhits10 += 1;
	  if(rank <= 5) {
	    randombreakhits5 += 1;
	    if(rank <= 3) {
	      randombreakhits3 += 1;
	      if(rank == 1) {
		randombreakhits1 += 1;
	      }
	    }
	  }
	}
      }
      randombreakmrr += 1.0 / rank;
      randombreakmr += (double)rank;

      rank = rankings[i][j];
      if(rank <= 20) {
	hits20 += 1;
	if(rank <= 10) {
	  hits10 += 1;
	  if(rank <= 5) {
	    hits5 += 1;
	    if(rank <= 3) {
	      hits3 += 1;
	      if(rank == 1) {
		hits1 += 1;
	      }
	    }
	  }
	}
      }
      mrr += 1.0 / rank;
      mr += (double)rank;
    }
  }

  double toprinthits10 = randombreakhits10;
  double toprinthits3 = randombreakhits3;
  double toprinthits1 = randombreakhits1;
  double toprintmrr = randombreakmrr;
  double toprintmr = randombreakmr;

  if(counter > 0) {
    aggressivehits20 /= counter;
    aggressivehits10 /= counter;
    aggressivehits5 /= counter;
    aggressivehits3 /= counter;
    aggressivehits1 /= counter;
    aggressivemrr /= counter;
    aggressivemr /= counter;

    randombreakhits20 /= counter;
    randombreakhits10 /= counter;
    randombreakhits5 /= counter;
    randombreakhits3 /= counter;
    randombreakhits1 /= counter;
    randombreakmrr /= counter;
    randombreakmr /= counter;

    midpointhits20 /= counter;
    midpointhits10 /= counter;
    midpointhits5 /= counter;
    midpointhits3 /= counter;
    midpointhits1 /= counter;
    midpointmrr /= counter;
    midpointmr /= counter;

    hits20 /= counter;
    hits10 /= counter;
    hits5 /= counter;
    hits3 /= counter;
    hits1 /= counter;
    mrr /= counter;
    mr /= counter;
  }

  //  outfile<<"---------------------------------"<<endl;
  int rankingType = params_.getRankingType(); // 0 is aggresive, 1 is intermediate, 2 is conservative 
  string stoprint;
  if(rankingType == 0)
    stoprint = "Aggressive MidPoint RandomBreak Aggressive";
  else if(rankingType == 1)
    stoprint = "Aggressive MidPoint RandomBreak Intermediate";
  else if(rankingType == 2)
    stoprint = "Aggressive MidPoint RandomBreak Conservative";
  else if(rankingType == 3)
    stoprint = "Aggressive MidPoint RandomBreak RandomBreak";
  if(isFiltered)
    outfile<<"Statistics Filtered ("<<stoprint<<"):"<<endl;
  else
    outfile<<"Statistics Raw ("<<stoprint<<"):"<<endl;
  outfile<<"HITS@1: "<<aggressivehits1<<" "<<midpointhits1<<" "<<randombreakhits1<<" "<<hits1<<endl;
  outfile<<"HITS@3: "<<aggressivehits3<<" "<<midpointhits3<<" "<<randombreakhits3<<" "<<hits3<<endl;
  outfile<<"HITS@10: "<<aggressivehits10<<" "<<midpointhits10<<" "<<randombreakhits10<<" "<<hits10<<endl;
  outfile<<"MRR: "<<aggressivemrr<<" "<<midpointmrr<<" "<<randombreakmrr<<" "<<mrr<<endl;
  outfile<<"MR: "<<aggressivemr<<" "<<midpointmr<<" "<<randombreakmr<<" "<<mr<<endl;

  outfile.precision(17);

   if(isFiltered) {
    if (params_.getReportStatsEntity())
      outfile<<"rd_mr_mrr_1_3_10_n_rf "<<fixed<<toprintmr<<" "<<toprintmrr<<" "<<toprinthits1<<" "<<toprinthits3<<" "<<toprinthits10<<" "<<counter<<" "<<numRuleFired_<<endl;
    else
      outfile<<"relation_mr_mrr_1_3_10_n_rf "<<fixed<<toprintmr<<" "<<toprintmrr<<" "<<toprinthits1<<" "<<toprinthits3<<" "<<toprinthits10<<" "<<counter<<" "<<numRuleFired_<<endl;
  }

  outfile.close();
}
// THIS FUNCTION IS CURRENTLY JUST WORKING FOR LEFT AND RIGHT COMBINED
void Solver::computeStatisticsForRelations(string fname, vector<vector<int> >& rankingsAggressive, vector<vector<int> >& rankingsMidPoint, vector<vector<int> >& rankingsRandomBreak, vector<vector<int> >& rankings, bool isFiltered)
{
  ofstream outfile(fname.c_str(), std::ios_base::app);

  outfile<<"MRR for each relation"<<endl;
  int rankingType = params_.getRankingType(); // 0 is aggresive, 1 is intermediate, 2 is conservative 
  string stoprint;
  if(rankingType == 0)
    stoprint = "Aggressive MidPoint RandomBreak Aggressive";
  else if(rankingType == 1)
    stoprint = "Aggressive MidPoint RandomBreak Intermediate";
  else if(rankingType == 2)
    stoprint = "Aggressive MidPoint RandomBreak Conservative";
  else if(rankingType == 3)
    stoprint = "Aggressive MidPoint RandomBreak RandomBreak";
  if(isFiltered)
    outfile<<"Statistics Filtered ("<<stoprint<<"):"<<endl;
  else
    outfile<<"Statistics Raw ("<<stoprint<<"):"<<endl;

  // Since this function is called for the combined LEFT and
  // RIGHT rankings, the vectors rankings are twice the size
  // of the vector relations, the first half is for LEFT
  // and the second half is for RIGHT
  vector<string>& relations = data_.getRelations();
  int numRelations = (int)relations.size();
  assert(numRelations * 2 == rankings.size());
  for(int relId=0; relId<(int)relations.size(); relId++) {
    if(rankings[relId].size()==0) continue;
    double aggressivehits1 = 0.0;
    double aggressivehits3 = 0.0;
    double aggressivehits5 = 0.0;
    double aggressivehits10 = 0.0;
    double aggressivehits20 = 0.0;
    double aggressivemrr = 0.0;
    double midpointhits1 = 0.0;
    double midpointhits3 = 0.0;
    double midpointhits5 = 0.0;
    double midpointhits10 = 0.0;
    double midpointhits20 = 0.0;
    double midpointmrr = 0.0;
    double randombreakhits1 = 0.0;
    double randombreakhits3 = 0.0;
    double randombreakhits5 = 0.0;
    double randombreakhits10 = 0.0;
    double randombreakhits20 = 0.0;
    double randombreakmrr = 0.0;
    double hits1 = 0.0;
    double hits3 = 0.0;
    double hits5 = 0.0;
    double hits10 = 0.0;
    double hits20 = 0.0;
    double mrr = 0.0;
    int counter = 0;

    for(int iter=0; iter<2; iter++) {
      int i = relId + iter * numRelations;

      for(int j=0; j<(int)rankings[i].size(); j++) {
	counter++;

	int rank = rankingsAggressive[i][j];
	if(rank <= 20) {
	  aggressivehits20 += 1;
	  if(rank <= 10) {
	    aggressivehits10 += 1;
	    if(rank <= 5) {
	      aggressivehits5 += 1;
	      if(rank <= 3) {
		aggressivehits3 += 1;
		if(rank == 1) {
		  aggressivehits1 += 1;
		}
	      }
	    }
	  }
	}
	aggressivemrr += 1.0 / rank;

	rank = rankingsMidPoint[i][j];
	if(rank <= 20) {
	  midpointhits20 += 1;
	  if(rank <= 10) {
	    midpointhits10 += 1;
	    if(rank <= 5) {
	      midpointhits5 += 1;
	      if(rank <= 3) {
		midpointhits3 += 1;
		if(rank == 1) {
		  midpointhits1 += 1;
		}
	      }
	    }
	  }
	}
	midpointmrr += 1.0 / rank;

	rank = rankingsRandomBreak[i][j];
	if(rank <= 20) {
	  randombreakhits20 += 1;
	  if(rank <= 10) {
	    randombreakhits10 += 1;
	    if(rank <= 5) {
	      randombreakhits5 += 1;
	      if(rank <= 3) {
		randombreakhits3 += 1;
		if(rank == 1) {
		  randombreakhits1 += 1;
		}
	      }
	    }
	  }
	}
	randombreakmrr += 1.0 / rank;

	rank = rankings[i][j];
	if(rank <= 20) {
	  hits20 += 1;
	  if(rank <= 10) {
	    hits10 += 1;
	    if(rank <= 5) {
	      hits5 += 1;
	      if(rank <= 3) {
		hits3 += 1;
		if(rank == 1) {
		  hits1 += 1;
		}
	      }
	    }
	  }
	}
	mrr += 1.0 / rank;
      }

    }

    aggressivehits20 /= counter;
    aggressivehits10 /= counter;
    aggressivehits5 /= counter;
    aggressivehits3 /= counter;
    aggressivehits1 /= counter;
    aggressivemrr /= counter;

    midpointhits20 /= counter;
    midpointhits10 /= counter;
    midpointhits5 /= counter;
    midpointhits3 /= counter;
    midpointhits1 /= counter;
    midpointmrr /= counter;

    randombreakhits20 /= counter;
    randombreakhits10 /= counter;
    randombreakhits5 /= counter;
    randombreakhits3 /= counter;
    randombreakhits1 /= counter;
    randombreakmrr /= counter;

    hits20 /= counter;
    hits10 /= counter;
    hits5 /= counter;
    hits3 /= counter;
    hits1 /= counter;
    mrr /= counter;

      //    outfile<<"HITS@1: "<<aggressivehits1<<" "<<randombreakhits1<<" "<<hits1<<endl;
      //    outfile<<"HITS@3: "<<aggressivehits3<<" "<<randombreakhits3<<" "<<hits3<<endl;
      // outfile<<"HITS@5: "<<aggressivehits5<<" "<<randombreakhits5<<" "<<hits5<<endl;
      //    outfile<<"HITS@10: "<<aggressivehits10<<" "<<randombreakhits10<<" "<<hits10<<endl;
      // outfile<<"HITS@20: "<<aggressivehits20<<" "<<randombreakhits20<<" "<<hits20<<endl;
      //    outfile<<"MRR: "<<aggressivemrr<<" "<<randombreakmrr<<" "<<mrr<<endl;
    outfile<<relId<<" "<<relations[relId]<<": "<<aggressivemrr<<" "<<midpointmrr<<" "<<randombreakmrr<<" "<<mrr<<endl;

  }
  outfile.close();
}


void Solver::generateRules(int relationId, vector<Rule>& rules)
{
  vector<string>& relations = data_.getRelations();
  int nrelations = (int) relations.size();
  //  int relationId = params_.getRelationId();
  int maxRuleLength = params_.getMaxRuleLength();
  bool useRelationInRules = params_.getUseRelationInRules();
  bool useRulesOfLengthOne = params_.getUseRulesOfLengthOne();
  bool useReverseArcsInRules = params_.getUseReverseArcsInRules();

  assert(rules.size() == 0);
  assert(maxRuleLength > 0);

  // generate rules of length one
  if(useRulesOfLengthOne) {
    bool useRelation = params_.getUseRelationInLengthOneRule();
    for(int i=0; i<nrelations; i++) {
      if(i == relationId && !useRelation)
	continue; // do not add rule since it has relationId
      if(relationId >= nrelations && i == relationId-nrelations && !useRelation)
	continue; // do not add rule since it has -relationId-1
      Rule rule;
      rule.addRelationId(i);
      rules.push_back(rule);
      if(useReverseArcsInRules) {
	Rule rule;
	rule.addRelationId(i,true);
	rules.push_back(rule);
      }
    }
  }

  int start = 0;
  int end = (int)rules.size();
  assert((useRulesOfLengthOne && end>0) || (!useRulesOfLengthOne && end==0));

  // generate rules of length greater than one
  for(int i=1; i<maxRuleLength; i++) {
    if(i==1) { // create rules of length 2
      for(int j=0; j<nrelations; j++) {
	for(int k=0; k<nrelations; k++) {
	  if(!useRelationInRules && (j==relationId || k==relationId))
	    continue; // do not create a rule that has relationId
	  if(relationId>=nrelations && !useRelationInRules && (j==relationId-nrelations || k==relationId-nrelations))
	    continue; // do not create a rule that has -relationId-1
	  Rule rule;
	  rule.addRelationId(j);
	  rule.addRelationId(k);
	  assert(rule.getLengthRule() == i+1);
	  rules.push_back(rule);
	  if(useReverseArcsInRules) {
	    {
	      Rule rule;
	      rule.addRelationId(j,true);
	      rule.addRelationId(k);
	      assert(rule.getLengthRule() == i+1);
	      rules.push_back(rule);
	    }
	    {
	      Rule rule;
	      rule.addRelationId(j);
	      rule.addRelationId(k,true);
	      assert(rule.getLengthRule() == i+1);
	      rules.push_back(rule);
	    }
	    {
	      Rule rule;
	      rule.addRelationId(j,true);
	      rule.addRelationId(k,true);
	      assert(rule.getLengthRule() == i+1);
	      rules.push_back(rule);
	    }
	  }
	}
      }
      start = end;
      end = (int)rules.size();
    }
    else {
      for(int j=start; j<end; j++) {
	for(int k=0; k<nrelations; k++) {
	  if(!useRelationInRules && k==relationId)
	    continue; // do not create a rule that has relationId
	  if(relationId>=nrelations && !useRelationInRules && k==relationId-nrelations)
	    continue; // do not create a rule that has -relationId-1
	  vector<int>& relationIds = rules[j].getRelationIds();
	  vector<bool>& isReverseArc = rules[j].getIsReverseArc();
	  Rule rule;
	  for(int l=0; l<(int)relationIds.size(); l++) {
	    rule.addRelationId(relationIds[l],isReverseArc[l]);
	  }
	  rule.addRelationId(k);
	  assert(rule.getLengthRule() == i+1);
	  rules.push_back(rule);
	  if(useReverseArcsInRules) {
	    vector<int>& relationIds = rules[j].getRelationIds();
	    vector<bool>& isReverseArc = rules[j].getIsReverseArc();
	    Rule rule;
	    for(int l=0; l<(int)relationIds.size(); l++) {
	      rule.addRelationId(relationIds[l],isReverseArc[l]);
	    }
	    rule.addRelationId(k,true);
	    assert(rule.getLengthRule() == i+1);
	    rules.push_back(rule);
	  }
	}
      }
      start = end;
      end = (int)rules.size();
    }
  }

}

void Solver::generateRulesHeuristic(int relationId, vector<Rule>& rules)
{
  vector<double> duals;
  generateRulesHeuristic(relationId, duals, rules);
}

void Solver::generateRulesHeuristic(int relationId,
				    vector<double>& duals,
				    vector<Rule>& rules)
{
  vector<string>& relations = data_.getRelations();
  int nrelations = (int) relations.size();
  int maxRuleLength = params_.getMaxRuleLength();
  bool useRelationInRules = params_.getUseRelationInRules();
  bool useRulesOfLengthOne = params_.getUseRulesOfLengthOne();
  bool useReverseArcsInRules = params_.getUseReverseArcsInRules();
  bool useRelationInLengthOneRule = params_.getUseRelationInLengthOneRule();
  assert(maxRuleLength > 0);

  Query& query = data_.getQuery(relationId);
  int numPairsInQuery = data_.getNumPairsQuery(relationId);

  vector<pair<int,int> >& pairs = query.getEntityPairs();

  if(duals.size()==0)
    duals.resize(numPairsInQuery,1.0);

  vector<int> pairIds(numPairsInQuery); // if pairIds[i]=-1, pair i is not being considered anymore
  vector<int> nodeIds(numPairsInQuery);
  vector<int> endNodeStarts(numPairsInQuery);
  vector<int> lengths(numPairsInQuery);
  for(int i=0; i<numPairsInQuery; i++) {
    pairIds[i] = i;
    nodeIds[i] = pairs[i].first;
  }

  vector<int> relsIds;
  vector<int> invrelsIds;
  int numOutRelations = 10;
  getHighFrequencyRelations(relationId,numOutRelations,nodeIds,
			    duals,relsIds,invrelsIds);

  vector<Rule> temprules;
  for(int i=0; i<(int)relsIds.size(); i++) {
    Rule rule;
    rule.addRelationId(relsIds[i]);
    temprules.push_back(rule);
  }
  for(int i=0; i<(int)invrelsIds.size(); i++) {
    Rule rule;
    rule.addRelationId(invrelsIds[i],true);
    temprules.push_back(rule);
  }

  int maxNumEndNodes = 5;
  int numColsMatrix = maxNumEndNodes * maxRuleLength;
  vector<vector<vector<int> > > rulesToNodes;
  for(int i=0; i<(int)temprules.size(); i++) {
    vector<int> nodes;
    getEndNodesRule(temprules[i],nodeIds,endNodeStarts,lengths,nodes);
    vector<vector<int> > pairsToNodes(numPairsInQuery);
    for(int j=0; j<(int)pairsToNodes.size(); j++) {
      pairsToNodes[j].resize(numColsMatrix,-1);
    }
    for(int j=0; j<(int)nodeIds.size(); j++) {
      int sIndex = endNodeStarts[j];
      int eIndex = endNodeStarts[j]+lengths[j]-1;
      if(lengths[j]>maxNumEndNodes)
	eIndex = endNodeStarts[j]+maxNumEndNodes-1;
      int k=0;
      for(int l=sIndex; l<=eIndex; l++) {
	assert(k<maxNumEndNodes);
	pairsToNodes[j][k] = nodes[l];
	k++;
      }
    }
    assert(pairsToNodes.size()>0);
    rulesToNodes.push_back(pairsToNodes);
  }

  // repeat until reaching maxRuleLength
  for(int iter=1; iter<maxRuleLength; iter++) {
    for(int i=0; i<(int)temprules.size(); i++) {
      Rule& rule = temprules[i];
      relsIds.clear();
      invrelsIds.clear();
      numOutRelations = 1;
      vector<vector<int> >& pairsToNodes = rulesToNodes[i];
      for(int j=0; j<numPairsInQuery; j++) {
	int kStart = (iter-1)*maxNumEndNodes;
	int kEnd = kStart+maxNumEndNodes-1;
	int index = rand() % (kEnd+1-kStart) + kStart;
	assert(kStart<=index && index<=kEnd);

	// add only one node per query pair
	if(pairsToNodes[j][index]<0)
	  nodeIds[j]=pairsToNodes[j][kStart]; // pick the first one
	else
	  nodeIds[j]=pairsToNodes[j][index]; // it can be -1
      }
      getHighFrequencyRelations(relationId,numOutRelations,
				nodeIds,duals,relsIds,invrelsIds);
      assert((relsIds.size()==1 && invrelsIds.size()==0) ||
	     (relsIds.size()==0 && invrelsIds.size()==1));
      for(int j=0; j<(int)relsIds.size(); j++) {
	rule.addRelationId(relsIds[j]);
      }
      for(int j=0; j<(int)invrelsIds.size(); j++) {
	rule.addRelationId(invrelsIds[j],true);
      }
      vector<int> nodes;
      assert(endNodeStarts.size()==nodeIds.size());
      assert(lengths.size()==nodeIds.size());
      getEndNodesRule(rule,nodeIds,endNodeStarts,lengths,nodes);
      assert(pairsToNodes.size()==nodeIds.size());
      int kStart = iter*maxNumEndNodes;
      int kEnd = kStart+maxNumEndNodes-1;
      for(int j=0; j<(int)nodeIds.size(); j++) {
	int sIndex = endNodeStarts[j];
	if(sIndex>=(int)nodes.size())
	  continue;
	int eIndex = endNodeStarts[j]+lengths[j]-1;
	if(lengths[j]>maxNumEndNodes)
	  eIndex = endNodeStarts[j]+maxNumEndNodes-1;
	assert(kStart<pairsToNodes[j].size());
	assert(kEnd<pairsToNodes[j].size());
	assert(sIndex<(int)nodes.size());
	assert(eIndex<(int)nodes.size());
	int k=kStart;
	for(int l=sIndex; l<=eIndex; l++) {
	  assert(k<=kEnd);
	  pairsToNodes[j][k] = nodes[l];
	  k++;
	}
      }
    }
  }

  // generate rules based on the rulesToNodes information
  int minNumReached = 0;
  for(int i=0; i<(int)temprules.size(); i++) {
    Rule& rule = temprules[i];
    vector<int>& relationIds = rule.getRelationIds();
    vector<bool>& isReverseArc = rule.getIsReverseArc();
    if(!useRulesOfLengthOne && rule.getLengthRule()==1)
      continue;
    if(!useRelationInLengthOneRule && rule.getLengthRule()==1 &&
       relationIds[0]==relationId)
      continue;
    bool doNotUseRule = false;
    for(int j=0; j<(int)relationIds.size(); j++) {
      if((!useRelationInRules && relationIds[j]==relationId) ||
	 (!useReverseArcsInRules && isReverseArc[j])) {
	doNotUseRule = true;
	break;
      }
    }
    if(doNotUseRule)
      continue;
    vector<vector<int> >& pairsToNodes = rulesToNodes[i];
    for(int iter=0; iter<maxRuleLength; iter++) {
      int numReached = 0;
      int kStart = iter*maxNumEndNodes;
      int kEnd = kStart+maxNumEndNodes-1;
      for(int j=0; j<numPairsInQuery; j++) {
	int head = pairs[j].second;
	for(int l=kStart; l<=kEnd; l++) {
	  if(pairsToNodes[j][l]==head)
	    numReached++;
	}
      }
      if(numReached >= minNumReached) {
	Rule newrule;
	for(int m=0; m<=iter; m++)
	  newrule.addRelationId(relationIds[m],isReverseArc[m]);
	bool addRule = true;
	for(int m=0; m<(int)rules.size(); m++) {
	  if(newrule == rules[m]) {
	    addRule = false;
	    break;
	  }
	}
	if(addRule)
	  rules.push_back(newrule);
      }
    }
  }

}

class NumRelationsMore {
public:
  int operator() (const pair<int,int>& x, const pair<int,int>& y) const {
    return (x.second > y.second);
  }
};

class WNumRelationsMore {
public:
  int operator() (const pair<int,double>& x, const pair<int,double>& y) const {
    return (x.second > y.second);
  }
};

void Solver::getHighFrequencyRelations(int relationId, int numOutRelations, vector<int>& nodeIds, vector<double>& duals, vector<int>& outrelations, vector<int>& outinvrelations)
{
  assert(outrelations.size()==0);
  assert(outinvrelations.size()==0);

  int nrelations = data_.getNumberRelations();
  bool useReverseArcs = params_.getUseReverseArcsInRules();

  vector<pair<int,double> > wNumRelations(nrelations);
  vector<pair<int,double> > wNumInvRelations(nrelations);
  for(int i=0; i<nrelations; i++) {
    wNumRelations[i] = pair<int,double>(i,0);
    wNumInvRelations[i] = pair<int,double>(i,0);
  }


  for (int i=0; i<(int)nodeIds.size(); i++){
    int nodeId = nodeIds[i];
    if(nodeId < 0) continue;
    for (int j=0; j<data_.getOutArcs()[nodeId].size(); j++){
      int rel = data_.getOutArcs()[nodeId][j]->getIdRelation(); 
      
      if (rel == relationId)
	continue;

      wNumRelations[rel].second += duals[i];
    }

    if (useReverseArcs){
      for (int j=0; j<data_.getInArcs()[nodeId].size(); j++){
	int rel = data_.getInArcs()[nodeId][j]->getIdRelation(); 
	
	if (rel == relationId)
	  continue;

	wNumInvRelations[rel].second += duals[i];
      }
    }
  }

  sort(wNumRelations.begin(),wNumRelations.end(),WNumRelationsMore());
  sort(wNumInvRelations.begin(),wNumInvRelations.end(),WNumRelationsMore());

  int i = 0;
  int j = 0;
  int nAdded = 0;
  while(nAdded < numOutRelations) {
    if(wNumRelations[i].second >= wNumInvRelations[j].second) {
      outrelations.push_back(wNumRelations[i].first);
      i++;
    }
    else {
      outinvrelations.push_back(wNumInvRelations[j].first);
      j++;
    }
    nAdded++;
  }

}

void Solver::getEndNodesRule(Rule& rule, vector<int>& nodeIds, vector<int>& endNodeStarts, vector<int>& lengths, vector<int>& endNodeIds)
{
  // it is assumed that the nodeIds are the beginning nodes
  // of the last relation in the rule
  // endNodeIds are going to be the end nodes that can
  // be reached by the last relation in the rule
  // if nodeIds[i]=-1, it means that there is no node
  int lastRelationId = rule.getRelationIds().back();
  bool isReverseArc = rule.getIsReverseArc().back();

  for (int i=0; i<(int)nodeIds.size(); i++){
    int nodeId = nodeIds[i];
    endNodeStarts[i] = (int)endNodeIds.size();
    if(nodeId>=0) {
      if(!isReverseArc) {
	for (int j=0; j<data_.getOutArcs()[nodeId].size(); j++){
	  int rel = data_.getOutArcs()[nodeId][j]->getIdRelation(); 
	  if (rel == lastRelationId) {
	    int headId = data_.getOutArcs()[nodeId][j]->getHead()->getId();
	    endNodeIds.push_back(headId);
	  }
	}
      }
      else {
	int nodeId = nodeIds[i];
	for (int j=0; j<data_.getInArcs()[nodeId].size(); j++){
	  int rel = data_.getInArcs()[nodeId][j]->getIdRelation(); 
	  if (rel == lastRelationId) {
	    int tailId = data_.getInArcs()[nodeId][j]->getTail()->getId();
	    endNodeIds.push_back(tailId);
	  }
	}
      }
    }
    lengths[i] = (int)endNodeIds.size()-endNodeStarts[i];
  }
}

void Solver::writeRulesToFile(int relationId, string fname)
{
  ofstream outfile(fname.c_str(), std::ios_base::app);

  string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int numrelations = data_.getNumberRelations();
  vector<string>& relations = data_.getRelations();
  for(int i=0; i<(int)rulesselected_[relationId].size(); i++) {
    if(rulesselected_[relationId][i] > 0) {
      Rule& rule = rules_[relationId][rulesadded_[relationId][i]];
      int len = rule.getLengthRule();
      assert(len < alphabet.length());
      outfile<<rulesweights_[relationId][i]<<"\t";
      if(relationId >= numrelations)
	outfile<<relations[relationId-numrelations]<<"("<<alphabet[len]<<",A) <=";
      else
	outfile<<relations[relationId]<<"(A,"<<alphabet[len]<<") <=";
      vector<int>& relIds = rule.getRelationIds();
      vector<bool>& isReverseArc = rule.getIsReverseArc();
      for(int j=0; j<len; j++) {
	if(j==0)
	  outfile<<" ";
	else
	  outfile<<", ";
	outfile<<relations[relIds[j]];
	if(isReverseArc[j])
	  outfile<<"("<<alphabet[j+1]<<","<<alphabet[j] <<")";
	else
	  outfile<<"("<<alphabet[j]<<","<<alphabet[j+1] <<")";
      }
      outfile<<endl;
    }
  }

  outfile.close();
}

void separateRelationsAndEntities(string input, vector<string>& output)
{
  istringstream ss( input );
  string s;
  getline( ss, s, '(' );
  output.push_back(s);
  getline( ss, s, ',' );
  output.push_back(s);
  getline( ss, s, ')' );
  output.push_back(s);

}

void separateStrings(string input, vector<vector<string> >& output)
{
  string s;
  string stemp1, stemp2, stemp3;
  istringstream ss( input );
  int counter=1;
  vector<string> values;
  while (ss) {
    string s;
    if (!getline( ss, s, ' ' )) break;
    if(s.compare("<=")==0) continue;
    if(s.back() == ',') s.pop_back();
    values.push_back(s);
    counter++;
  }
  for(int i=0; i<(int)values.size(); i++) {
    vector<string> relents;
    separateRelationsAndEntities(values[i], relents);
    assert(relents.size() > 0);
    output.push_back(relents);
  }
}

void Solver::readRulesFromFile(string fname)
{
  ifstream infile(fname.c_str());

  int numrelations = data_.getNumberRelations();
  vector<string>& relations = data_.getRelations();
  map<string,int>& maprelations = data_.getMapRelations();

  string s;
  while (getline( infile, s )) {
    istringstream ss( s );
    vector<string> values;
    while (ss) {
      string s;
      if (!getline( ss, s, '\t' )) break;
      values.push_back(s);
    }
    double weight = atof(values[0].c_str());
    vector<vector<string> > output;
    separateStrings(values[1], output);
    if(output.size()<=1) continue; // some lines don't have rules
    // relation
    int relationId = -1;
    map<string,int>::iterator it = maprelations.find(output[0][0]);
    if (it != maprelations.end())
      relationId = it->second;
    if (params_.getReportStatsEntity()) {
      if(params_.getRunOnlyWithRelationId() && relationId != params_.getRelationId()) {
        continue;
      }
    }
    string firstent = output[0][1];
    Rule rule;
    for(int i=1; i<(int)output.size(); i++) {
      int newrelationId = -1;
      it = maprelations.find(output[i][0]);
      if (it != maprelations.end())
	newrelationId = it->second;
      bool isReverse;
      if(output[i][1] == firstent) {
	isReverse = false;
	firstent = output[i][2];
      }
      else {
	isReverse = true;
	firstent = output[i][1];
      }
      rule.addRelationId(newrelationId, isReverse);
    }
    rules_[relationId].push_back(rule);
    int index = (int)rules_[relationId].size() - 1;
    rulesadded_[relationId].push_back(index);
    rulesselected_[relationId].push_back(1.0);
    rulesweights_[relationId].push_back(weight);
  }

  infile.close();
}

void Solver::readAnyBURLRulesFromFile(string fname)
{
  ifstream infile(fname.c_str());

  int numrelations = data_.getNumberRelations();
  vector<string>& relations = data_.getRelations();
  map<string,int>& maprelations = data_.getMapRelations();


  string s;
  string stemp1, stemp2, stemp3;
  while (getline( infile, s )) {
    istringstream ss( s );
    int counter=1;
    vector<string> values;
    while (ss) {
      string s;
      if (!getline( ss, s, '\t' )) break;
      values.push_back(s);
      counter++;
    }
    for(int i=0; i<(int)values.size(); i++)
      cout<<values[i]<<" : ";
    cout<<endl;
    double weight = atof(values[2].c_str());
    cout<<"weight: "<<weight<<endl;
    vector<vector<string> > output;
    separateStrings(values[3], output);
    if(output.size()<=1) continue; // some lines don't have rules
    // relation
    int relationId = -1;
    map<string,int>::iterator it = maprelations.find(output[0][0]);
    if (it != maprelations.end())
      relationId = it->second;
    string firstent = output[0][1];
    Rule rule;
    for(int i=1; i<(int)output.size(); i++) {
      int newrelationId = -1;
      it = maprelations.find(output[i][0]);
      if (it != maprelations.end())
	newrelationId = it->second;
      bool isReverse;
      if(output[i][1] == firstent) {
	isReverse = false;
	firstent = output[i][2];
      }
      else {
	isReverse = true;
	firstent = output[i][1];
      }
      rule.addRelationId(newrelationId, isReverse);
    }

    int length = rule.getLengthRule();
    cout<<"new rule for relation "<<output[0][0]<<" ("<<relationId<<"): ";
    vector<int>& relIds = rule.getRelationIds();
    vector<bool>& isRev = rule.getIsReverseArc();
    for(int k=0; k<length; k++) {
      if(isRev[k])
	cout<<"R_"<<relations[relIds[k]]<<" ("<<relIds[k]<<") ";
      else
	cout<<relations[relIds[k]]<<" ("<<relIds[k]<<") ";
    }
    cout<<endl;

    rules_[relationId].push_back(rule);
    int index = (int)rules_[relationId].size() - 1;
    rulesadded_[relationId].push_back(index);
    rulesselected_[relationId].push_back(1.0);
    rulesweights_[relationId].push_back(weight);
  }

  infile.close();
}