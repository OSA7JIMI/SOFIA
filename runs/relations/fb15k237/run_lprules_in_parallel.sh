# © Copyright IBM Corporation 2022. All Rights Reserved.
# LICENSE: Eclipse Public License - v 2.0, https://opensource.org/licenses/EPL-2.0
# SPDX-License-Identifier: EPL-2.0


#!/bin/bash
# to run type: 
# ./run_lprules_in_parallel.sh parameterFile nameForOutputFiles numberOfRelations 
# The results are written in the file results_nameForOutputFiles.txt

# Create folders to store output
mkdir -p v1 v2 v3 v4

# Parameter filename
PARAMS=p_fb.txt

relations=(180 200 215 219)

case "$1" in
    v1) versions=(1) ;;
    v2) versions=(2) ;;
    v3) versions=(3) ;;
    v4) versions=(4) ;;
    all|"") versions=(1 2 3 4) ;;
    *)
        echo "Invalid argument: $1. Choose from: v1, v2, v3, v4, all"
        exit 1
        ;;
esac

for i in "${versions[@]}"
do
# write data path to file
DATAPATH="data_directory ../../../data/inductivefb15k237/v$i"
echo >> $PARAMS
echo $DATAPATH >> $PARAMS

# Output name
OUTNAME=outfbv$i
# Number of Relations
NRELATIONS=${relations[i-1]}
# Scores filename
SCORES=v$i/scores_$OUTNAME
# Rules filename
RULES=v$i/rules_$OUTNAME
# Input rules filename
INPUTRULES=v$i/rules_all.txt
# Forward chain filename
FORWARDCHAIN=v$i/induced_facts.txt
# Log filename
LOG=v$i/log_$OUTNAME
# Results filename
RESULTS=results_$OUTNAME.txt
# Executable
EXEC=../../../code/lprules

NRELATIONS=$((NRELATIONS-1))
for p in $(seq 0 $NRELATIONS)
do
    $EXEC -p $PARAMS -s $SCORES-p$p.txt -r $RULES-p$p.txt -i $p -v $INPUTRULES -f $FORWARDCHAIN > $LOG-p$p.txt &
done

wait

echo "========================================" > $RESULTS
echo "ENTITY PREDICTION RESULTS" >> $RESULTS
echo "========================================" >> $RESULTS

if grep -q "rd_mr_mrr_1_3_10_n_rf" $SCORES* 2>/dev/null; then
    grep rd_mr_mrr_1_3_10_n_rf $SCORES* >> $RESULTS
    grep rd_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[2]; sum1+=a[7]} END {print "MR " sum/sum1}' >> $RESULTS
    grep rd_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[3]; sum1+=a[7]} END {print "MRR " sum/sum1}' >> $RESULTS
    grep rd_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[4]; sum1+=a[7]} END {print "HITS@1 " 100*sum/sum1}' >> $RESULTS
    grep rd_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[8]; sum1+=a[7]} END {print "Coverage " 100*sum/sum1}' >> $RESULTS
    grep rd_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[5]; sum1+=a[7]} END {print "HITS@3 " 100*sum/sum1}' >> $RESULTS
    grep rd_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[6]; sum1+=a[7]} END {print "HITS@10 " 100*sum/sum1}' >> $RESULTS
else
    echo "Relation prediction in progress: switch to entity prediction using report_stats_entities." >> $RESULTS
fi

echo "========================================" >> $RESULTS
echo "RELATION PREDICTION RESULTS" >> $RESULTS
echo "========================================" >> $RESULTS

if grep -q "relation_mr_mrr_1_3_10_n_rf" $SCORES* 2>/dev/null; then
    grep relation_mr_mrr_1_3_10_n_rf $SCORES* >> $RESULTS
    grep relation_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[2]; sum1+=a[7]} END {print "MR " sum/sum1}' >> $RESULTS
    grep relation_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[3]; sum1+=a[7]} END {print "MRR " sum/sum1}' >> $RESULTS
    grep relation_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[4]; sum1+=a[7]} END {print "HITS@1 " 100*sum/sum1}' >> $RESULTS
    grep relation_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[8]; sum1+=a[7]} END {print "Coverage " 100*sum/sum1}' >> $RESULTS
    grep relation_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[5]; sum1+=a[7]} END {print "HITS@3 " 100*sum/sum1}' >> $RESULTS
    grep relation_mr_mrr_1_3_10_n_rf $SCORES* | awk -v OFMT=%.17g '{split($0,a," "); sum += a[6]; sum1+=a[7]} END {print "HITS@10 " 100*sum/sum1}' >> $RESULTS
else
    echo "If relation prediction training completed: set run_mode to 1 to get results." >> $RESULTS
    echo "Else if entity prediction completed: switch to relation prediction using report_stats_relations." >> $RESULTS
fi

sed -i '$d' $PARAMS

cat v$i/rules*.txt > v$i/rules_all.txt

done

SUMMARY=results_summary.txt

if grep -q "relation_mr_mrr_1_3_10_n_rf" $SCORES* 2>/dev/null; then

    echo "Summary" > $SUMMARY
    echo "" >> $SUMMARY

    echo -n "MRR:      " >> $SUMMARY
    for i in {1..4}; do
        OUTNAME=outfbv$i
        RESULTS=results_$OUTNAME.txt
        MRR=$(grep "^MRR " $RESULTS 2>/dev/null | awk '{print $2}')
        printf "&%s  " "$MRR" >> $SUMMARY
    done
    echo "" >> $SUMMARY

    echo -n "HITS@1:   " >> $SUMMARY
    for i in {1..4}; do
        OUTNAME=outfbv$i
        RESULTS=results_$OUTNAME.txt
        H=$(grep "^HITS@1 " $RESULTS 2>/dev/null | awk '{print $2}')
        printf "&%s  " "$H" >> $SUMMARY
    done
    echo "" >> $SUMMARY

    echo -n "Coverage: " >> $SUMMARY
    for i in {1..4}; do
        OUTNAME=outfbv$i
        RESULTS=results_$OUTNAME.txt
        Coverage=$(grep "^Coverage " $RESULTS 2>/dev/null | awk '{print $2}')
        printf "&%s  " "$Coverage" >> $SUMMARY
    done
    echo "" >> $SUMMARY

else
    echo "" > $SUMMARY
fi