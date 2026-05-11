for p in $(seq 1 4)
do
    awk -F'\t' '{print $1; print $3}' v$p/*_v$p/{train,valid,test}.txt | sort -u | awk '{print $0"\t"NR-1}' > v$p/fb237_v$p/entity2id.txt
    awk -F'\t' '{print $2}' v$p/*_v$p/{train,valid,test}.txt | sort -u | awk '{print $0"\t"NR-1}' > v$p/fb237_v$p/relation2id.txt

done

