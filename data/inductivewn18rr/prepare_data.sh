for p in $(seq 1 4)
do
    cat v$p/*_v${p}_ind/{train,valid,test}.txt > v$p/test.txt
    cat v$p/*_v$p/{train,valid,test}.txt > v$p/train.txt
    awk -F'\t' '{print $1; print $3}' v$p/train.txt | sort -u | awk '{print $0"\t"NR-1}' > v$p/entity2id.txt
    awk -F'\t' '{print $1; print $3}' v$p/test.txt | sort -u | awk '{print $0"\t"NR-1}' > v$p/test_entity2id.txt
    awk -F'\t' '{print $2}' v$p/train.txt v$p/test.txt | sort -u | awk '{print $0"\t"NR-1}' > v$p/relation2id.txt

done

