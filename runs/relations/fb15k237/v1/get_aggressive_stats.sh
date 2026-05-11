echo >> results_outfb.txt

awk '
/Statistics Filtered.*Aggressive/ { in_section=1; next }
in_section && /HITS@1:/ { hits1=$2; next }
in_section && /HITS@3:/ { hits3=$2; next }
in_section && /HITS@10:/ { hits10=$2; next }
in_section && /MRR:/ { mrr=$2; next }
in_section && /MR:/ { mr=$2; in_section=0; next }
/relation_mr_mrr/ { 
    n=$(NF-1)
    if(hits1 != "") {
        sum_hits1 += hits1*n
        sum_hits3 += hits3*n
        sum_hits10 += hits10*n
        sum_mrr += mrr*n
        sum_mr += mr*n
        sum_n += n
        hits1=""
    }
}
END {
    if(sum_n > 0) {
        printf "MR %.5f\n", sum_mr/sum_n
        printf "MRR %.6f\n", sum_mrr/sum_n
        printf "HITS@1 %.4f\n", sum_hits1/sum_n * 100
        printf "HITS@3 %.4f\n", sum_hits3/sum_n * 100
        printf "HITS@10 %.4f\n", sum_hits10/sum_n * 100
    }
}
' scores_outfb*.txt >> results_outfb.txt