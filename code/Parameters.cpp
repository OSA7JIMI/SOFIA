// © Copyright IBM Corporation 2022. All Rights Reserved.
// LICENSE: Eclipse Public License - v 2.0, https://opensource.org/licenses/EPL-2.0
// SPDX-License-Identifier: EPL-2.0

#include "Parameters.hpp"

#include <iomanip>
#include <cassert>
#include <queue>

using namespace std;

void Parameters::initialize()
{
  relationId_ = -1;
  penaltyOnNegativePairs_.push_back(0.0);
  runMode_ = 0;

  useRelationInRules_ = true;
  useRulesOfLengthOne_ = true;
  useRelationInLengthOneRule_ = false;
  penaltyOnComplexity_ = 0.03;
  printScoresToFile_ = false;
  writeLpFile_ = false;
  rankingType_ = 1; // 0 is aggresive, 1 is intermediate, 2 is conservative, 3 is randomBreak
  useReverseArcsInRules_ = true;
  runOnlyForRelationsInTest_ = false;
  runForReverseRelations_ = false;
  generateRules_ = 1; // 0 is enumeration, 1 is S0, 2 is heuristic
  runFindBestComplexity_ = true;
  findBestComplexityRankingType_ = 3; // 0 is aggresive, 1 is intermediate, 2 is conservative, 3 is randomBreak
  addPenaltyOnNegativePairs_ = false;
  useBreadthFirstSearch_ = false;
  repeatedNodesAllowed_ = false;

  runOnlyWithRelationId_ = false;  
  useFrequencyBias_ = false;
  useSoftMargin_  = false;
  marginValue_ = 0.1;
  slackPenalty_ = 0.01;

  useSpecificityWeights_ = false;

  useRelaxedRules_ = false;
  relaxedWeight_ = 0.1;
  reportStatsEntity_= false;
  reportStatsRelation_= false;

  minForwardChainScore_ = 0.0;
  useForwardChainGraph_ = false;
  forwardChainSize_ = 1;
  maxChainSize_ = 200000;
}

void Parameters::readParamsFile(string fname)
{
  string s;
  string stemp1, stemp2, stemp3;

  ifstream infile(fname.c_str());

  while (getline( infile, s )) {
    int ent1, ent2;
    istringstream ss( s );
    //    ss >> stemp1 >> stemp2;
    int counter=1;
    vector<string> extraValues;
    while (ss) {
      string s;
      if (!getline( ss, s, ' ' )) break;
      if(counter==1)
	stemp1 = s;
      else if(counter==2)
	stemp2 = s;
      else if(counter>2)
	extraValues.push_back(s);
      counter++;
    }
    if(stemp1 == "data_directory")
      directory_ = stemp2;
    else if(stemp1 == "max_complexity")
      maxComplexity_ =  atoi(stemp2.c_str());
    else if(stemp1 == "relation_id")
      relationId_ = atoi(stemp2.c_str());
    else if(stemp1 == "use_relation_in_rules") {
      if(stemp2 == "true")
	useRelationInRules_ = true;
      else
	useRelationInRules_ = false;
    }
    else if(stemp1 == "use_rules_of_length_one") {
      if(stemp2 == "true")
	useRulesOfLengthOne_ = true;
      else
	useRulesOfLengthOne_ = false;
    }
    else if(stemp1 == "use_relation_in_length_one_rule")
      if(stemp2 == "true")
	useRelationInLengthOneRule_ = true;
      else
	useRelationInLengthOneRule_ = false;
    else if(stemp1 == "max_rule_length")
      maxRuleLength_ = atoi(stemp2.c_str());
    else if(stemp1 == "penalty_on_complexity")
      penaltyOnComplexity_ =  atof(stemp2.c_str());
    else if(stemp1 == "print_scores_to_file") {
      if(stemp2 == "true")
	printScoresToFile_ = true;
      else
	printScoresToFile_ = false;
    }
    else if(stemp1 == "write_lp_file") {
      if(stemp2 == "true")
	writeLpFile_ = true;
      else
	writeLpFile_ = false;
    }
    else if(stemp1 == "ranking_type") {
      rankingType_ =  atoi(stemp2.c_str());
      findBestComplexityRankingType_ = rankingType_;
    }
    else if(stemp1 == "use_reverse_arcs_in_rules") {
      if(stemp2 == "true")
	useReverseArcsInRules_ = true;
      else
	useReverseArcsInRules_ = false;
    }
    else if(stemp1 == "run_only_for_relations_in_test") {
      if(stemp2 == "true")
	runOnlyForRelationsInTest_ = true;
      else
	runOnlyForRelationsInTest_ = false;
    }
    else if(stemp1 == "run_for_reverse_relations") {
      if(stemp2 == "true")
	runForReverseRelations_ = true;
      else
	runForReverseRelations_ = false;
    }
    else if(stemp1 == "generate_rules")
      generateRules_ =  atoi(stemp2.c_str());
    else if(stemp1 == "run_find_best_complexity") {
      if(stemp2 == "true")
	runFindBestComplexity_ = true;
      else
	runFindBestComplexity_ = false;
    }
    else if(stemp1 == "find_best_complexity_ranking_type")
      findBestComplexityRankingType_ =  atoi(stemp2.c_str());
    else if(stemp1 == "penalty_on_negative_pairs") {
      //      penaltyOnNegativePairs_ =  atof(stemp2.c_str());
      penaltyOnNegativePairs_[0] = atof(stemp2.c_str());
      for(int i=0; i<(int)extraValues.size(); i++)
	penaltyOnNegativePairs_.push_back(atof(extraValues[i].c_str()));
      if(penaltyOnNegativePairs_.size()>1 || penaltyOnNegativePairs_[0]>0.0)
	addPenaltyOnNegativePairs_ = true;
    }
    else if(stemp1 == "use_breadth_first_search") {
      if(stemp2 == "true")
	useBreadthFirstSearch_ = true;
      else
	useBreadthFirstSearch_ = false;
    }
    else if(stemp1 == "repeated_nodes_allowed") {
      if(stemp2 == "true")
	repeatedNodesAllowed_ = true;
      else
	repeatedNodesAllowed_ = false;
    }
    else if(stemp1 == "run_mode") {
      runMode_ =  atoi(stemp2.c_str());
    }
    else if(stemp1 == "use_frequency_bias") {
      if(stemp2 == "true")
	useFrequencyBias_ = true;
      else
	useFrequencyBias_ = false;
    }

    else if(stemp1 == "use_soft_margin") {
      if(stemp2 == "true")
	useSoftMargin_ = true;
      else
	useSoftMargin_ = false;
    }

    else if(stemp1 == "margin_value") {
           marginValue_ =  atof(stemp2.c_str());
    }
    else if(stemp1 == "slack_penalty") {
           slackPenalty_ =  atof(stemp2.c_str());
    }

    else if(stemp1 == "use_specificity_weights") {
      if(stemp2 == "true")
	useSpecificityWeights_ = true;
      else
	useSpecificityWeights_ = false;
    }

    else if(stemp1 == "use_relaxed_rules") {
      if(stemp2 == "true")
	useRelaxedRules_ = true;
      else
	useRelaxedRules_ = false;
    }
     else if(stemp1 == "relaxed_weight") {
           relaxedWeight_ = atof(stemp2.c_str());
    }

    else if(stemp1 == "use_transductive_setting") {
      if(stemp2 == "true")
	transductiveSetting_ = true;
      else
	transductiveSetting_ = false;
    }
    else if(stemp1 == "report_stats_entities") {
      if(stemp2 == "true")
	reportStatsEntity_ = true;
      else
	reportStatsEntity_ = false;
    }
    else if(stemp1 == "report_stats_relations") {
    if(stemp2 == "true")
      reportStatsRelation_ = true;
    else
      reportStatsRelation_ = false;
  }

  else if(stemp1 == "min_forward_chain_score") {
           minForwardChainScore_ = atof(stemp2.c_str());
    }

   else if(stemp1 == "use_forward_chain_graph") {
      if(stemp2 == "true")
	useForwardChainGraph_ = true;
      else
	useForwardChainGraph_ = false;
    }

    else if(stemp1 == "forward_chain_size") {
           forwardChainSize_ = atoi(stemp2.c_str());
    }

    else if(stemp1 == "max_chain_size") {
           maxChainSize_ = atoi(stemp2.c_str());
    }

  }

  if(useRelationInRules_ == false)
    useRelationInLengthOneRule_ = false;   

  if(reportStatsRelation_) {
    reportStatsEntity_ = false;
  }

  if(useSoftMargin_ == false)
    useSpecificityWeights_ = false;
  else {
    penaltyOnNegativePairs_.assign(1, 0.0);
    addPenaltyOnNegativePairs_ = false;
  }

  if(relationId_ >= 0)
    runOnlyWithRelationId_ = true;
}

void Parameters::printParams()
{
  cout<<"-------------------------"<<endl;
  cout<<"Parameters:"<<endl;
  cout<<"data_directory "<<directory_<<endl;
  cout<<"max_complexity "<<maxComplexity_<<endl;
  cout<<"relation_id "<<relationId_<<endl;
  if(useRelationInRules_)
    cout<<"use_relation_in_rules true"<<endl;
  else
    cout<<"use_relation_in_rules false"<<endl;
  if(useRulesOfLengthOne_)
    cout<<"use_rules_of_length_one true"<<endl;
  else
    cout<<"use_rules_of_length_one false"<<endl;
  if(useRelationInLengthOneRule_)
    cout<<"use_relation_in_length_one_rule true"<<endl;
  else
    cout<<"use_relation_in_length_one_rule false"<<endl;
  cout<<"max_rule_length "<<maxRuleLength_<<endl;
  cout<<"penalty_on_complexity "<<penaltyOnComplexity_<<endl;
  if(printScoresToFile_)
      cout<<"print_scores_to_file true"<<endl;
  else
    cout<<"print_scores_to_file false"<<endl;
  if(writeLpFile_)
    cout<<"write_lp_file true"<<endl;
  else
    cout<<"write_lp_file false"<<endl;
  cout<<"ranking_type "<<rankingType_<<endl;
  if(useReverseArcsInRules_)
    cout<<"use_reverse_arcs_in_rules true"<<endl;
  else
    cout<<"use_reverse_arcs_in_rules false"<<endl;
  if(runOnlyForRelationsInTest_)
    cout<<"run_only_for_relations_in_test true"<<endl;
  else
    cout<<"run_only_for_relations_in_test false"<<endl;
  if(runForReverseRelations_)
    cout<<"run_for_reverse_relations true"<<endl;
  else
    cout<<"run_for_reverse_relations false"<<endl;
  cout<<"generate_rules "<<generateRules_<<endl;
  if(runFindBestComplexity_)
    cout<<"run_find_best_complexity true"<<endl;
  else
    cout<<"run_find_best_complexity false"<<endl;
  cout<<"find_best_complexity_ranking_type "<<findBestComplexityRankingType_<<endl;
   cout<<"add_penalty_on_negative_pairs "<<addPenaltyOnNegativePairs_<<endl;
  cout<<"penalty_on_negative_pairs";
  for(int i=0; i<(int)penaltyOnNegativePairs_.size(); i++)
    cout<<" "<<penaltyOnNegativePairs_[i];
  cout<<endl;
  if(useBreadthFirstSearch_)
    cout<<"use_breadth_first_search true"<<endl;
  else
    cout<<"use_breadth_first_search false"<<endl;
  if(repeatedNodesAllowed_)
    cout<<"repeated_nodes_allowed true"<<endl;
  else
    cout<<"repeated_nodes_allowed false"<<endl;
  cout<<"run_mode "<<runMode_<<endl;
  cout<<"use_frequency_bias "<<useFrequencyBias_<<endl;
  cout<<"use_soft_margin "<<useSoftMargin_<<endl;
  cout<<"margin_value "<<marginValue_<<endl;
  cout<<"slack_penalty "<<slackPenalty_<<endl;
  cout<<"use_specificity_weights "<<useSpecificityWeights_<<endl;
  cout<<"use_relaxed_rules "<<useRelaxedRules_<<endl; 
  cout<<"relaxed_weight "<<relaxedWeight_<<endl; 
  cout<<"use_transductive_setting "<<transductiveSetting_<<endl;
  cout<<"min_forward_chain_score "<<minForwardChainScore_<<endl;
  cout<<"use_forward_chain_graph "<<useForwardChainGraph_<<endl;
  cout<<"forward_chain_size "<<forwardChainSize_<<endl;
  cout<<"max_chain_size "<<maxChainSize_<<endl;
    cout<<"-------------------------"<<endl;  
}